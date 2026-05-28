import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { BroadcastError } from "#/api/broadcast/errors";
import { useSendMessage } from "#/hooks/use-send-message";

const mocks = vi.hoisted(() => ({
	sendMessage: vi.fn().mockResolvedValue({ messageId: "msg-1" }),
}));

vi.mock("#/api/broadcast/operations", () => ({
	sendMessage: mocks.sendMessage,
}));

describe("useSendMessage", () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it("sendMessage calls rpcSendMessage with correct args", async () => {
		const { result } = renderHook(() => useSendMessage());

		await act(async () => {
			await result.current.sendMessage("client-1", "channel-1", "hello");
		});

		expect(mocks.sendMessage).toHaveBeenCalledWith(
			"client-1",
			"channel-1",
			"hello",
		);
	});

	it("sendMessage throws BroadcastError on failure", async () => {
		const { Code, ConnectError } = await import("@connectrpc/connect");
		mocks.sendMessage.mockRejectedValueOnce(
			new ConnectError("fail", Code.Unavailable),
		);
		const { result } = renderHook(() => useSendMessage());

		await expect(
			act(async () => {
				await result.current.sendMessage("client-1", "channel-1", "hello");
			}),
		).rejects.toBeInstanceOf(BroadcastError);
	});
});
