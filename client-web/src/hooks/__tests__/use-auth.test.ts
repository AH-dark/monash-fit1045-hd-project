import { createElement, type PropsWithChildren } from "react";

import { Code, ConnectError } from "@connectrpc/connect";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { BroadcastError } from "@/api/broadcast/errors";
import { useAuth } from "@/hooks/use-auth";
import { useAuthStore } from "@/stores/auth-store";

const mocks = vi.hoisted(() => ({
	connect: vi.fn().mockResolvedValue({ clientId: "test-id" }),
	disconnect: vi.fn().mockResolvedValue(undefined),
}));

vi.mock("@/api/broadcast/operations", () => ({
	connect: mocks.connect,
	disconnect: mocks.disconnect,
}));

function wrapper({ children }: PropsWithChildren) {
	const queryClient = new QueryClient({
		defaultOptions: { mutations: { retry: false } },
	});
	return createElement(QueryClientProvider, { client: queryClient }, children);
}

describe("useAuth", () => {
	beforeEach(() => {
		useAuthStore.getState().reset();
		vi.clearAllMocks();
	});

	it("connect calls rpcConnect and updates auth store on success", async () => {
		const { result } = renderHook(() => useAuth(), { wrapper });

		await act(async () => {
			const connectPromise = result.current.connect("alice");

			expect(useAuthStore.getState().status).toBe("connecting");

			await connectPromise;
		});

		expect(mocks.connect).toHaveBeenCalledWith("alice");
		expect(useAuthStore.getState().clientId).toBe("test-id");
		expect(useAuthStore.getState().username).toBe("alice");
		expect(useAuthStore.getState().status).toBe("connected");
	});

	it("connect calls reset on ClientNotFound error", async () => {
		mocks.connect.mockRejectedValueOnce(
			new ConnectError("client not found", Code.NotFound),
		);
		useAuthStore.getState().setConnected("stale-id", "alice");
		const { result } = renderHook(() => useAuth(), { wrapper });

		await expect(
			act(async () => {
				await result.current.connect("alice");
			}),
		).rejects.toThrow();

		expect(useAuthStore.getState().status).toBe("disconnected");
		expect(useAuthStore.getState().clientId).toBeNull();
		expect(useAuthStore.getState().username).toBeNull();
	});

	it("connect maps ConnectError to BroadcastError and disconnects on non-client errors", async () => {
		mocks.connect.mockRejectedValueOnce(
			new ConnectError("server unavailable", Code.Unavailable),
		);
		const { result } = renderHook(() => useAuth(), { wrapper });

		let thrown: unknown;
		await act(async () => {
			try {
				await result.current.connect("alice");
			} catch (err) {
				thrown = err;
			}
		});

		expect(thrown).toBeInstanceOf(BroadcastError);
		expect((thrown as BroadcastError).kind).toBe("unavailable");
		expect(useAuthStore.getState().status).toBe("disconnected");
		expect(useAuthStore.getState().clientId).toBeNull();
		expect(useAuthStore.getState().username).toBeNull();
	});

	it("disconnect calls rpcDisconnect and calls setDisconnected", async () => {
		useAuthStore.getState().setConnected("test-id", "alice");
		const { result } = renderHook(() => useAuth(), { wrapper });

		await act(async () => {
			await result.current.disconnect("test-id");
		});

		expect(mocks.disconnect).toHaveBeenCalledWith("test-id");
		expect(useAuthStore.getState().status).toBe("disconnected");
		expect(useAuthStore.getState().clientId).toBeNull();
	});

	it("disconnect resets auth state when client is not found", async () => {
		mocks.disconnect.mockRejectedValueOnce(
			new ConnectError("client not found", Code.NotFound),
		);
		useAuthStore.getState().setConnected("stale-id", "alice");
		const { result } = renderHook(() => useAuth(), { wrapper });

		await expect(
			act(async () => {
				await result.current.disconnect("stale-id");
			}),
		).rejects.toThrow();

		expect(useAuthStore.getState().status).toBe("disconnected");
		expect(useAuthStore.getState().clientId).toBeNull();
		expect(useAuthStore.getState().username).toBeNull();
	});
});
