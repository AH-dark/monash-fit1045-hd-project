import { renderHook, act } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { useAuth } from "#/hooks/use-auth";
import { useAuthStore } from "#/stores/auth-store";

const mocks = vi.hoisted(() => ({
	connect: vi.fn().mockResolvedValue({ clientId: "test-id" }),
	disconnect: vi.fn().mockResolvedValue(undefined),
}));

vi.mock("#/api/broadcast/operations", () => ({
	connect: mocks.connect,
	disconnect: mocks.disconnect,
}));

describe("useAuth", () => {
	beforeEach(() => {
		useAuthStore.getState().reset();
		vi.clearAllMocks();
	});

	it("connect calls rpcConnect and updates auth store on success", async () => {
		const { result } = renderHook(() => useAuth());

		await act(async () => {
			await result.current.connect("alice");
		});

		expect(mocks.connect).toHaveBeenCalledWith("alice");
		expect(useAuthStore.getState().clientId).toBe("test-id");
		expect(useAuthStore.getState().username).toBe("alice");
		expect(useAuthStore.getState().status).toBe("connected");
	});

	it("connect calls reset on ClientNotFound error", async () => {
		const { Code, ConnectError } = await import("@connectrpc/connect");
		mocks.connect.mockRejectedValueOnce(
			new ConnectError("client not found", Code.NotFound),
		);
		const { result } = renderHook(() => useAuth());

		await expect(
			act(async () => {
				await result.current.connect("alice");
			}),
		).rejects.toThrow();

		expect(useAuthStore.getState().status).toBe("disconnected");
		expect(useAuthStore.getState().clientId).toBeNull();
		expect(useAuthStore.getState().username).toBeNull();
	});

	it("disconnect calls rpcDisconnect and calls setDisconnected", async () => {
		useAuthStore.getState().setConnected("test-id", "alice");
		const { result } = renderHook(() => useAuth());

		await act(async () => {
			await result.current.disconnect();
		});

		expect(mocks.disconnect).toHaveBeenCalledWith("test-id");
		expect(useAuthStore.getState().status).toBe("disconnected");
		expect(useAuthStore.getState().clientId).toBeNull();
	});
});
