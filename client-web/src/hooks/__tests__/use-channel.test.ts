import { createElement, type PropsWithChildren } from "react";

import { Code, ConnectError } from "@connectrpc/connect";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
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
	subscribeToChannel: vi.fn(),
}));

vi.mock("@/api/broadcast/operations", () => ({
	joinChannel: mocks.joinChannel,
	subscribeToChannel: mocks.subscribeToChannel,
}));

function wrapper({ children }: PropsWithChildren) {
	const queryClient = new QueryClient({
		defaultOptions: { queries: { retry: false } },
	});
	return createElement(QueryClientProvider, { client: queryClient }, children);
}

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
		mocks.subscribeToChannel.mockReturnValue({
			[Symbol.asyncIterator]: () => ({ next: () => new Promise(() => {}) }),
		});
		vi.clearAllMocks();
	});

	it("returns messages and isConnected", () => {
		const { result } = renderHook(
			() => useChannel({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper },
		);

		expect(result.current.messages).toEqual([]);
		expect(typeof result.current.isConnected).toBe("boolean");
	});

	it("isConnected is false initially", () => {
		const { result } = renderHook(
			() => useChannel({ clientId: null, channelId: "channel-1" }),
			{ wrapper },
		);

		expect(result.current.isConnected).toBe(false);
		expect(result.current.messages).toEqual([]);
	});

	it("streams events into Zustand and keeps isConnected true while active", async () => {
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

		const { result } = renderHook(
			() => useChannel({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper },
		);

		await waitFor(() => expect(addMessageSpy).toHaveBeenCalledTimes(2));
		expect(result.current.messages).toHaveLength(2);
		expect(result.current.isConnected).toBe(true);
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

	it("resets auth on client-not-found errors", async () => {
		const resetSpy = vi.spyOn(useAuthStore.getState(), "reset");
		mocks.subscribeToChannel.mockReturnValueOnce({
			[Symbol.asyncIterator]: () => ({
				async next() {
					throw new ConnectError("client missing", Code.NotFound);
				},
			}),
		});

		renderHook(
			() => useChannel({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper },
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

		const { unmount } = renderHook(
			() => useChannel({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper },
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
});
