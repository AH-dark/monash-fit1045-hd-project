import { createElement, type PropsWithChildren } from "react";

import { Code, ConnectError } from "@connectrpc/connect";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { BroadcastError } from "@/api/broadcast/errors";
import { useLeaveChannelMutation } from "@/hooks/use-leave-channel-mutation";

const mocks = vi.hoisted(() => ({
	leaveChannel: vi.fn().mockResolvedValue(undefined),
}));

vi.mock("@/api/broadcast/operations", () => ({
	leaveChannel: mocks.leaveChannel,
}));

function wrapper({ children }: PropsWithChildren) {
	const queryClient = new QueryClient({
		defaultOptions: { mutations: { retry: false } },
	});
	return createElement(QueryClientProvider, { client: queryClient }, children);
}

describe("useLeaveChannelMutation", () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it("calls leaveChannel with mutation variables", async () => {
		const { result } = renderHook(() => useLeaveChannelMutation(), { wrapper });

		await act(async () => {
			await result.current.mutateAsync({
				clientId: "client-1",
				channelId: "channel-1",
			});
		});

		expect(mocks.leaveChannel).toHaveBeenCalledWith("client-1", "channel-1");
	});

	it("maps ConnectError(NotFound, channel message) to BroadcastError with kind channel-not-found", async () => {
		const connectErr = new ConnectError("channel not found", Code.NotFound);
		mocks.leaveChannel.mockRejectedValueOnce(connectErr);

		const { result } = renderHook(() => useLeaveChannelMutation(), { wrapper });

		let thrown: unknown;
		await act(async () => {
			try {
				await result.current.mutateAsync({
					clientId: "client-1",
					channelId: "channel-1",
				});
			} catch (err) {
				thrown = err;
			}
		});

		expect(thrown).toBeInstanceOf(BroadcastError);
		expect((thrown as BroadcastError).kind).toBe("channel-not-found");
	});

	it("maps ConnectError(NotFound, client message) to BroadcastError with kind client-not-found", async () => {
		const connectErr = new ConnectError("client not found", Code.NotFound);
		mocks.leaveChannel.mockRejectedValueOnce(connectErr);

		const { result } = renderHook(() => useLeaveChannelMutation(), { wrapper });

		let thrown: unknown;
		await act(async () => {
			try {
				await result.current.mutateAsync({
					clientId: "client-1",
					channelId: "channel-1",
				});
			} catch (err) {
				thrown = err;
			}
		});

		expect(thrown).toBeInstanceOf(BroadcastError);
		expect((thrown as BroadcastError).kind).toBe("client-not-found");
	});
});
