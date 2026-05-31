import { Code, ConnectError } from "@connectrpc/connect";
import { act, renderHook, waitFor } from "@testing-library/react";
import { beforeEach, describe, expect, expectTypeOf, it, vi } from "vitest";

import type { ChannelEvent, MessageEvent } from "@/gen/bcmd/v1/broadcast_pb.ts";
import { useChannel } from "@/hooks/use-channel";
import type { Message } from "@/schemas/message";
import { useAuthStore } from "@/stores/auth-store";
import { useMessagesStore } from "@/stores/messages-store";

const emptyMessages: [] = [];

const mocks = vi.hoisted(() => ({
	joinChannel: vi.fn().mockResolvedValue(undefined),
	leaveChannel: vi.fn().mockResolvedValue(undefined),
	subscribeToChannel: vi.fn(),
}));

vi.mock("@/api/broadcast/operations", () => ({
	joinChannel: mocks.joinChannel,
	leaveChannel: mocks.leaveChannel,
	subscribeToChannel: mocks.subscribeToChannel,
}));

function createDeferred<T>() {
	let resolve!: (value: T | PromiseLike<T>) => void;
	const promise = new Promise<T>((res) => {
		resolve = res;
	});
	return { promise, resolve };
}

function makeMessageEvent(overrides: Partial<MessageEvent> = {}): ChannelEvent {
	return {
		event: {
			case: "message",
			value: {
				messageId: "msg-1",
				channelId: "channel-1",
				senderId: "sender-1",
				senderName: "Ada",
				content: "hello",
				sentAtMs: 123n,
				fromReplay: false,
				...overrides,
			},
		},
	} as ChannelEvent;
}

function makeStream(events: ChannelEvent[], afterYield?: Promise<void>) {
	return {
		async *[Symbol.asyncIterator]() {
			for (const event of events) {
				yield event;
			}
			if (afterYield) {
				await afterYield;
			}
		},
	};
}

describe("useChannel", () => {
	beforeEach(() => {
		useMessagesStore.setState({
			messages: new Map([["channel-1", emptyMessages]]),
		});
		useAuthStore.setState({
			status: "connected",
			clientId: "client-1",
			username: "Ada",
		});
		mocks.joinChannel.mockResolvedValue(undefined);
		mocks.leaveChannel.mockResolvedValue(undefined);
		mocks.subscribeToChannel.mockReturnValue({
			[Symbol.asyncIterator]: () => ({ next: () => new Promise(() => {}) }),
		});
		vi.clearAllMocks();
	});

	it("returns messages and isConnected", () => {
		const { result } = renderHook(() =>
			useChannel({ clientId: "client-1", channelId: "channel-1" }),
		);

		expect(result.current.messages).toEqual([]);
		expect(typeof result.current.isConnected).toBe("boolean");
	});

	it("isConnected is false when clientId is null", () => {
		const { result } = renderHook(() =>
			useChannel({ clientId: null, channelId: "channel-1" }),
		);

		expect(result.current.isConnected).toBe(false);
		expect(result.current.messages).toEqual([]);
		expect(mocks.joinChannel).not.toHaveBeenCalled();
	});

	it("calls joinChannel before subscribing", async () => {
		renderHook(() =>
			useChannel({ clientId: "client-1", channelId: "channel-1" }),
		);

		await waitFor(() => {
			expect(mocks.joinChannel).toHaveBeenCalledWith("client-1", "channel-1");
		});
		await waitFor(() => {
			expect(mocks.subscribeToChannel).toHaveBeenCalled();
		});
		expect(mocks.joinChannel.mock.invocationCallOrder[0]).toBeLessThan(
			mocks.subscribeToChannel.mock.invocationCallOrder[0],
		);
	});

	it("streams events into Zustand and flips isConnected to true after join", async () => {
		const finish = createDeferred<void>();
		mocks.subscribeToChannel.mockReturnValueOnce(
			makeStream(
				[
					makeMessageEvent(),
					makeMessageEvent({
						messageId: "msg-2",
						senderId: "sender-2",
						senderName: "Grace",
						content: "world",
						sentAtMs: 456n,
					}),
				],
				finish.promise,
			),
		);
		const addMessageSpy = vi.spyOn(useMessagesStore.getState(), "addMessage");

		const { result } = renderHook(() =>
			useChannel({ clientId: "client-1", channelId: "channel-1" }),
		);

		await waitFor(() => expect(addMessageSpy).toHaveBeenCalledTimes(2));
		await waitFor(() => expect(result.current.isConnected).toBe(true));
		expect(result.current.messages).toHaveLength(2);
		expect(addMessageSpy).toHaveBeenNthCalledWith(1, {
			messageId: "msg-1",
			channelId: "channel-1",
			senderId: "sender-1",
			senderName: "Ada",
			content: "hello",
			timestamp: 123,
		});
		expect(addMessageSpy).toHaveBeenNthCalledWith(2, {
			messageId: "msg-2",
			channelId: "channel-1",
			senderId: "sender-2",
			senderName: "Grace",
			content: "world",
			timestamp: 456,
		});
		expectTypeOf(result.current.messages).toEqualTypeOf<Message[]>();

		await act(async () => {
			finish.resolve();
		});
	});

	it("isConnected stays false when join is still pending", async () => {
		const joinDeferred = createDeferred<void>();
		mocks.joinChannel.mockReturnValueOnce(joinDeferred.promise);

		const { result } = renderHook(() =>
			useChannel({ clientId: "client-1", channelId: "channel-1" }),
		);

		await waitFor(() => expect(mocks.joinChannel).toHaveBeenCalled());
		expect(result.current.isConnected).toBe(false);

		await act(async () => {
			joinDeferred.resolve();
		});

		await waitFor(() => expect(result.current.isConnected).toBe(true));
	});

	it("isConnected becomes false when the stream ends", async () => {
		const finish = createDeferred<void>();
		mocks.subscribeToChannel.mockReturnValueOnce(
			makeStream([makeMessageEvent()], finish.promise),
		);

		const { result } = renderHook(() =>
			useChannel({ clientId: "client-1", channelId: "channel-1" }),
		);

		await waitFor(() => expect(result.current.isConnected).toBe(true));

		await act(async () => {
			finish.resolve();
		});

		await waitFor(() => expect(result.current.isConnected).toBe(false));
	});

	it("resets auth on client-not-found errors", async () => {
		const resetSpy = vi.spyOn(useAuthStore.getState(), "reset");
		mocks.subscribeToChannel.mockReturnValueOnce({
			[Symbol.asyncIterator]: () => ({
				async next() {
					throw new ConnectError("client missing", Code.NotFound);
				},
			}),
		});

		renderHook(() =>
			useChannel({ clientId: "client-1", channelId: "channel-1" }),
		);

		await waitFor(() => expect(resetSpy).toHaveBeenCalledTimes(1));
	});

	it("passes an AbortSignal to subscribeToChannel and aborts on unmount", async () => {
		const abortObserved = createDeferred<void>();
		mocks.subscribeToChannel.mockReturnValueOnce({
			async *[Symbol.asyncIterator]() {
				const signal = mocks.subscribeToChannel.mock.calls[0]?.[3]?.signal;
				if (signal) {
					await new Promise<void>((resolve) => {
						signal.addEventListener("abort", resolve, { once: true });
					});
					abortObserved.resolve();
				}
			},
		});

		const { unmount } = renderHook(() =>
			useChannel({ clientId: "client-1", channelId: "channel-1" }),
		);

		await waitFor(() => expect(mocks.subscribeToChannel).toHaveBeenCalled());
		const call = mocks.subscribeToChannel.mock.calls[0];
		expect(call[0]).toBe("client-1");
		expect(call[1]).toBe("channel-1");
		expect(call[3]).toEqual(
			expect.objectContaining({ signal: expect.any(AbortSignal) }),
		);

		await act(async () => {
			unmount();
		});

		expect(call[3]?.signal.aborted).toBe(true);
		await expect(abortObserved.promise).resolves.toBeUndefined();
	});

	it("forwards replayCount to subscribeToChannel", async () => {
		renderHook(() =>
			useChannel({
				clientId: "client-1",
				channelId: "channel-1",
				replayCount: 25,
			}),
		);

		await waitFor(() => expect(mocks.subscribeToChannel).toHaveBeenCalled());
		const call = mocks.subscribeToChannel.mock.calls[0];
		expect(call[2]).toBe(25);
	});

	it("calls leaveChannel on unmount", async () => {
		const { unmount } = renderHook(() =>
			useChannel({ clientId: "client-1", channelId: "channel-1" }),
		);

		await waitFor(() => expect(mocks.joinChannel).toHaveBeenCalled());

		await act(async () => {
			unmount();
		});

		expect(mocks.leaveChannel).toHaveBeenCalledWith("client-1", "channel-1");
	});

	it("calls leaveChannel when switching channelId", async () => {
		const { rerender } = renderHook(
			({ channelId }: { channelId: string }) =>
				useChannel({ clientId: "client-1", channelId }),
			{ initialProps: { channelId: "channel-1" } },
		);

		await waitFor(() =>
			expect(mocks.joinChannel).toHaveBeenCalledWith("client-1", "channel-1"),
		);

		await act(async () => {
			rerender({ channelId: "channel-2" });
		});

		await waitFor(() =>
			expect(mocks.leaveChannel).toHaveBeenCalledWith("client-1", "channel-1"),
		);
		await waitFor(() =>
			expect(mocks.joinChannel).toHaveBeenCalledWith("client-1", "channel-2"),
		);
	});
});
