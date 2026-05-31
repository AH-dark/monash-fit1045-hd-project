import { createElement, type PropsWithChildren } from "react";

import { Code, ConnectError } from "@connectrpc/connect";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { act, renderHook, waitFor } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import type { ChannelEvent, MessageEvent } from "@/gen/bcmd/v1/broadcast_pb.ts";
import { useChannelMessages } from "@/hooks/use-channel-messages";
import { useAuthStore } from "@/stores/auth-store";

const mocks = vi.hoisted(() => ({
	joinChannel: vi.fn().mockResolvedValue(undefined),
	subscribeToChannel: vi.fn(),
	listMessages: vi.fn().mockResolvedValue({ messages: [], hasMore: false }),
}));

vi.mock("@/api/broadcast/operations", () => ({
	joinChannel: mocks.joinChannel,
	subscribeToChannel: mocks.subscribeToChannel,
	listMessages: mocks.listMessages,
}));

function createWrapper() {
	const queryClient = new QueryClient({
		defaultOptions: { queries: { retry: false } },
	});
	const Wrapper = ({ children }: PropsWithChildren) =>
		createElement(QueryClientProvider, { client: queryClient }, children);
	return { Wrapper, queryClient };
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
				sentAtMs: 100n,
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
			if (afterYield) await afterYield;
		},
	};
}

describe("useChannelMessages", () => {
	beforeEach(() => {
		useAuthStore.setState({
			status: "connected",
			clientId: "client-1",
			username: "Ada",
		});
		mocks.joinChannel.mockResolvedValue(undefined);
		mocks.subscribeToChannel.mockReturnValue({
			[Symbol.asyncIterator]: () => ({ next: () => new Promise(() => {}) }),
		});
		mocks.listMessages.mockResolvedValue({ messages: [], hasMore: false });
		vi.clearAllMocks();
	});

	it("returns empty messages and disconnected state when clientId is null", () => {
		const { Wrapper } = createWrapper();
		const { result } = renderHook(
			() => useChannelMessages({ clientId: null, channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		expect(result.current.messages).toEqual([]);
		expect(result.current.isConnected).toBe(false);
		expect(mocks.joinChannel).not.toHaveBeenCalled();
		expect(mocks.listMessages).not.toHaveBeenCalled();
	});

	it("calls joinChannel before subscribing to the stream", async () => {
		const { Wrapper } = createWrapper();
		renderHook(
			() =>
				useChannelMessages({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		await waitFor(() =>
			expect(mocks.joinChannel).toHaveBeenCalledWith("client-1", "channel-1"),
		);
		await waitFor(() => expect(mocks.subscribeToChannel).toHaveBeenCalled());
		expect(mocks.joinChannel.mock.invocationCallOrder[0]).toBeLessThan(
			mocks.subscribeToChannel.mock.invocationCallOrder[0],
		);
	});

	it("flips isConnected to true after join resolves", async () => {
		const join = createDeferred<void>();
		mocks.joinChannel.mockReturnValueOnce(join.promise);
		const { Wrapper } = createWrapper();
		const { result } = renderHook(
			() =>
				useChannelMessages({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		await waitFor(() => expect(mocks.joinChannel).toHaveBeenCalled());
		expect(result.current.isConnected).toBe(false);

		await act(async () => {
			join.resolve();
		});

		await waitFor(() => expect(result.current.isConnected).toBe(true));
	});

	it("loads history via listMessages and exposes it in messages", async () => {
		mocks.listMessages.mockResolvedValueOnce({
			messages: [
				{
					messageId: "h1",
					channelId: "channel-1",
					senderId: "alice",
					senderName: "Alice",
					content: "earlier",
					timestamp: 10,
				},
				{
					messageId: "h2",
					channelId: "channel-1",
					senderId: "alice",
					senderName: "Alice",
					content: "later",
					timestamp: 20,
				},
			],
			hasMore: false,
		});
		const { Wrapper } = createWrapper();
		const { result } = renderHook(
			() =>
				useChannelMessages({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		await waitFor(() => expect(result.current.messages.length).toBe(2));
		expect(result.current.messages.map((m) => m.messageId)).toEqual([
			"h1",
			"h2",
		]);
	});

	it("injects live stream messages into the query cache", async () => {
		const finish = createDeferred<void>();
		mocks.listMessages.mockResolvedValueOnce({
			messages: [
				{
					messageId: "h1",
					channelId: "channel-1",
					senderId: "alice",
					senderName: "Alice",
					content: "history",
					timestamp: 10,
				},
			],
			hasMore: false,
		});
		mocks.subscribeToChannel.mockReturnValueOnce(
			makeStream(
				[
					makeMessageEvent({
						messageId: "live-1",
						content: "first live",
						sentAtMs: 50n,
					}),
					makeMessageEvent({
						messageId: "live-2",
						content: "second live",
						sentAtMs: 60n,
					}),
				],
				finish.promise,
			),
		);

		const { Wrapper } = createWrapper();
		const { result } = renderHook(
			() =>
				useChannelMessages({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		await waitFor(() => expect(result.current.messages.length).toBe(3));
		expect(result.current.messages.map((m) => m.messageId)).toEqual([
			"h1",
			"live-1",
			"live-2",
		]);

		await act(async () => {
			finish.resolve();
		});
	});

	it("deduplicates messages with the same id", async () => {
		mocks.listMessages.mockResolvedValueOnce({
			messages: [
				{
					messageId: "dup",
					channelId: "channel-1",
					senderId: "alice",
					senderName: "Alice",
					content: "same",
					timestamp: 50,
				},
			],
			hasMore: false,
		});
		const finish = createDeferred<void>();
		mocks.subscribeToChannel.mockReturnValueOnce(
			makeStream(
				[makeMessageEvent({ messageId: "dup", sentAtMs: 50n })],
				finish.promise,
			),
		);
		const { Wrapper } = createWrapper();
		const { result } = renderHook(
			() =>
				useChannelMessages({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		await waitFor(() => expect(result.current.messages.length).toBe(1));
		expect(result.current.messages[0].messageId).toBe("dup");

		await act(async () => {
			finish.resolve();
		});
	});

	it("sorts messages by timestamp regardless of arrival order", async () => {
		const finish = createDeferred<void>();
		mocks.listMessages.mockResolvedValueOnce({
			messages: [
				{
					messageId: "a",
					channelId: "channel-1",
					senderId: "x",
					senderName: "X",
					content: "a",
					timestamp: 30,
				},
			],
			hasMore: false,
		});
		mocks.subscribeToChannel.mockReturnValueOnce(
			makeStream(
				[
					makeMessageEvent({ messageId: "b", sentAtMs: 50n, content: "b" }),
					makeMessageEvent({ messageId: "c", sentAtMs: 10n, content: "c" }),
				],
				finish.promise,
			),
		);
		const { Wrapper } = createWrapper();
		const { result } = renderHook(
			() =>
				useChannelMessages({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		await waitFor(() => expect(result.current.messages.length).toBe(3));
		expect(result.current.messages.map((m) => m.messageId)).toEqual([
			"c",
			"a",
			"b",
		]);

		await act(async () => {
			finish.resolve();
		});
	});

	it("exposes hasMore and fetchOlder backed by the infinite query", async () => {
		mocks.listMessages
			.mockResolvedValueOnce({
				messages: [
					{
						messageId: "p1",
						channelId: "channel-1",
						senderId: "alice",
						senderName: "Alice",
						content: "newest",
						timestamp: 50,
					},
				],
				hasMore: true,
			})
			.mockResolvedValueOnce({
				messages: [
					{
						messageId: "p0",
						channelId: "channel-1",
						senderId: "alice",
						senderName: "Alice",
						content: "older",
						timestamp: 10,
					},
				],
				hasMore: false,
			});

		const { Wrapper } = createWrapper();
		const { result } = renderHook(
			() =>
				useChannelMessages({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		await waitFor(() => expect(result.current.hasMore).toBe(true));
		expect(result.current.messages.map((m) => m.messageId)).toEqual(["p1"]);

		await act(async () => {
			result.current.fetchOlder();
		});

		await waitFor(() => expect(result.current.messages.length).toBe(2));
		expect(result.current.messages.map((m) => m.messageId)).toEqual([
			"p0",
			"p1",
		]);
		expect(result.current.hasMore).toBe(false);

		const secondCall = mocks.listMessages.mock.calls[1];
		expect(secondCall[2]).toBe("p1");
	});

	it("resets auth on client-not-found errors from the stream", async () => {
		const resetSpy = vi.spyOn(useAuthStore.getState(), "reset");
		mocks.subscribeToChannel.mockReturnValueOnce({
			[Symbol.asyncIterator]: () => ({
				async next() {
					throw new ConnectError("client missing", Code.NotFound);
				},
			}),
		});
		const { Wrapper } = createWrapper();
		renderHook(
			() =>
				useChannelMessages({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		await waitFor(() => expect(resetSpy).toHaveBeenCalled());
	});

	it("aborts the subscription stream on unmount", async () => {
		const abortObserved = createDeferred<void>();
		mocks.subscribeToChannel.mockReturnValueOnce({
			async *[Symbol.asyncIterator]() {
				const signal = mocks.subscribeToChannel.mock.calls[0]?.[2]?.signal;
				if (signal) {
					await new Promise<void>((resolve) => {
						signal.addEventListener("abort", resolve, { once: true });
					});
					abortObserved.resolve();
				}
			},
		});

		const { Wrapper } = createWrapper();
		const { unmount } = renderHook(
			() =>
				useChannelMessages({ clientId: "client-1", channelId: "channel-1" }),
			{ wrapper: Wrapper },
		);

		await waitFor(() => expect(mocks.subscribeToChannel).toHaveBeenCalled());

		await act(async () => {
			unmount();
		});

		const signal = mocks.subscribeToChannel.mock.calls[0]?.[2]?.signal;
		expect(signal?.aborted).toBe(true);
		await expect(abortObserved.promise).resolves.toBeUndefined();
	});

	it("switches channels by issuing a new history query and subscription", async () => {
		mocks.listMessages.mockImplementation((_clientId, channelId) =>
			Promise.resolve({
				messages: [
					{
						messageId: `${channelId}-msg`,
						channelId,
						senderId: "x",
						senderName: "X",
						content: channelId,
						timestamp: 1,
					},
				],
				hasMore: false,
			}),
		);

		const { Wrapper } = createWrapper();
		const { result, rerender } = renderHook(
			({ channelId }: { channelId: string }) =>
				useChannelMessages({ clientId: "client-1", channelId }),
			{ wrapper: Wrapper, initialProps: { channelId: "channel-1" } },
		);

		await waitFor(() =>
			expect(result.current.messages.map((m) => m.messageId)).toEqual([
				"channel-1-msg",
			]),
		);

		await act(async () => {
			rerender({ channelId: "channel-2" });
		});

		await waitFor(() =>
			expect(mocks.joinChannel).toHaveBeenCalledWith("client-1", "channel-2"),
		);
		await waitFor(() =>
			expect(result.current.messages.map((m) => m.messageId)).toEqual([
				"channel-2-msg",
			]),
		);
	});
});
