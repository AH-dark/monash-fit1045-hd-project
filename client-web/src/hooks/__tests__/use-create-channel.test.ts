import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { BroadcastError } from "@/api/broadcast/errors";
import { useCreateChannel } from "@/hooks/use-create-channel";
import { useChannelsStore } from "@/stores/channels-store";

const mocks = vi.hoisted(() => ({
	createChannel: vi.fn().mockResolvedValue({
		channelId: "ch-1",
		channelName: "test",
	}),
}));

vi.mock("@/api/broadcast/operations", () => ({
	createChannel: mocks.createChannel,
}));

describe("useCreateChannel", () => {
	beforeEach(() => {
		useChannelsStore.getState().reset();
		vi.clearAllMocks();
	});

	it("createChannel calls rpcCreateChannel with correct args", async () => {
		const { result } = renderHook(() => useCreateChannel());

		await act(async () => {
			await result.current.createChannel("client-1", "test-channel");
		});

		expect(mocks.createChannel).toHaveBeenCalledWith(
			"client-1",
			"test-channel",
		);
		expect(useChannelsStore.getState().channels.get("ch-1")).toEqual({
			id: "ch-1",
			name: "test",
			memberCount: 1,
		});
	});

	it("createChannel throws BroadcastError on failure", async () => {
		const { Code, ConnectError } = await import("@connectrpc/connect");
		mocks.createChannel.mockRejectedValueOnce(
			new ConnectError("fail", Code.Unavailable),
		);
		const { result } = renderHook(() => useCreateChannel());

		await expect(
			act(async () => {
				await result.current.createChannel("client-1", "test-channel");
			}),
		).rejects.toBeInstanceOf(BroadcastError);
	});
});
