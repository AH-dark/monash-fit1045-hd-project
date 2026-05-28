import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { BroadcastError } from "#/api/broadcast/errors";
import { useLeaveChannel } from "#/hooks/use-leave-channel";

const mocks = vi.hoisted(() => ({
	leaveChannel: vi.fn().mockResolvedValue(undefined),
}));

vi.mock("#/api/broadcast/operations", () => ({
	leaveChannel: mocks.leaveChannel,
}));

describe("useLeaveChannel", () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it("leaveChannel calls rpcLeaveChannel with correct args", async () => {
		const { result } = renderHook(() => useLeaveChannel());

		await act(async () => {
			await result.current.leaveChannel("client-1", "channel-1");
		});

		expect(mocks.leaveChannel).toHaveBeenCalledWith("client-1", "channel-1");
	});

	it("leaveChannel throws BroadcastError on failure", async () => {
		const { Code, ConnectError } = await import("@connectrpc/connect");
		mocks.leaveChannel.mockRejectedValueOnce(
			new ConnectError("fail", Code.Unavailable),
		);
		const { result } = renderHook(() => useLeaveChannel());

		await expect(
			act(async () => {
				await result.current.leaveChannel("client-1", "channel-1");
			}),
		).rejects.toBeInstanceOf(BroadcastError);
	});
});
