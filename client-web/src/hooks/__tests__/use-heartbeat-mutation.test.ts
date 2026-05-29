import { createElement, type PropsWithChildren } from "react";

import { Code, ConnectError } from "@connectrpc/connect";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { BroadcastError } from "@/api/broadcast/errors";
import { useHeartbeatMutation } from "@/hooks/use-heartbeat-mutation";

const mocks = vi.hoisted(() => ({
	heartbeat: vi.fn().mockResolvedValue(undefined),
}));

vi.mock("@/api/broadcast/operations", () => ({
	heartbeat: mocks.heartbeat,
}));

function wrapper({ children }: PropsWithChildren) {
	const queryClient = new QueryClient({
		defaultOptions: { mutations: { retry: false } },
	});
	return createElement(QueryClientProvider, { client: queryClient }, children);
}

describe("useHeartbeatMutation", () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it("calls heartbeat with clientId from variables object", async () => {
		const { result } = renderHook(() => useHeartbeatMutation(), { wrapper });

		await act(async () => {
			await result.current.mutateAsync({ clientId: "client-1" });
		});

		expect(mocks.heartbeat).toHaveBeenCalledWith("client-1");
	});

	it("maps ConnectError to BroadcastError on failure", async () => {
		const connectErr = new ConnectError("client not found", Code.NotFound);
		mocks.heartbeat.mockRejectedValueOnce(connectErr);

		const { result } = renderHook(() => useHeartbeatMutation(), { wrapper });

		let thrown: unknown;
		await act(async () => {
			try {
				await result.current.mutateAsync({ clientId: "client-1" });
			} catch (err) {
				thrown = err;
			}
		});

		expect(thrown).toBeInstanceOf(BroadcastError);
		expect((thrown as BroadcastError).kind).toBe("client-not-found");
	});
});
