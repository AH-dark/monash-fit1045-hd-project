import { createElement, type PropsWithChildren } from "react";

import { Code, ConnectError } from "@connectrpc/connect";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { renderHook, waitFor } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { BroadcastError } from "@/api/broadcast/errors";
import { subscribeToChannelList } from "@/api/broadcast/operations";
import type { ChannelListEvent } from "@/gen/bcmd/v1/broadcast_pb.ts";
import { useChannels } from "@/hooks/use-channels";
import { useAuthStore } from "@/stores/auth-store";
import { useChannelsStore } from "@/stores/channels-store";

const mocks = vi.hoisted(() => ({
	subscribeToChannelList: vi.fn().mockReturnValue({
		[Symbol.asyncIterator]: () => ({ next: () => new Promise(() => {}) }),
	}),
}));

vi.mock("@/api/broadcast/operations", () => ({
	subscribeToChannelList: mocks.subscribeToChannelList,
}));

function wrapper({ children }: PropsWithChildren) {
	const queryClient = new QueryClient({
		defaultOptions: { queries: { retry: false } },
	});
	return createElement(QueryClientProvider, { client: queryClient }, children);
}

function createChannelListStream(events: ReadonlyArray<ChannelListEvent>) {
	return {
		async *[Symbol.asyncIterator]() {
			for (const event of events) {
				yield event;
			}
			await new Promise(() => {});
		},
	};
}

describe("useChannels", () => {
	beforeEach(() => {
		useChannelsStore.getState().reset();
		useAuthStore.getState().reset();
		mocks.subscribeToChannelList.mockReturnValue({
			[Symbol.asyncIterator]: () => ({ next: () => new Promise(() => {}) }),
		});
		vi.clearAllMocks();
	});

	it("returns channels and snapshotApplied from store", () => {
		const { result } = renderHook(() => useChannels("client-1"), { wrapper });

		expect(result.current.channels).toBeInstanceOf(Map);
		expect(result.current.snapshotApplied).toBe(false);
	});

	it("dispatches snapshot events to applySnapshot", async () => {
		const applySnapshot = vi.spyOn(
			useChannelsStore.getState(),
			"applySnapshot",
		);
		const channels = [{ id: "channel-1", name: "General", memberCount: 1 }];
		mocks.subscribeToChannelList.mockReturnValue(
			createChannelListStream([
				{
					event: { case: "snapshot", value: { channels } },
				} as ChannelListEvent,
			]),
		);

		renderHook(() => useChannels("client-1"), { wrapper });

		await waitFor(() => {
			expect(applySnapshot).toHaveBeenCalledWith(channels);
		});
	});

	it("dispatches created events to applyCreated", async () => {
		const applyCreated = vi.spyOn(useChannelsStore.getState(), "applyCreated");
		const channel = { id: "channel-2", name: "Random", memberCount: 3 };
		mocks.subscribeToChannelList.mockReturnValue(
			createChannelListStream([
				{
					event: { case: "created", value: { channel } },
				} as ChannelListEvent,
			]),
		);

		renderHook(() => useChannels("client-1"), { wrapper });

		await waitFor(() => {
			expect(applyCreated).toHaveBeenCalledWith(channel);
		});
	});

	it("dispatches memberCountChanged events to applyMemberCountChanged", async () => {
		const applyMemberCountChanged = vi.spyOn(
			useChannelsStore.getState(),
			"applyMemberCountChanged",
		);
		mocks.subscribeToChannelList.mockReturnValue(
			createChannelListStream([
				{
					event: {
						case: "memberCountChanged",
						value: { channelId: "channel-1", memberCount: 7 },
					},
				} as ChannelListEvent,
			]),
		);

		renderHook(() => useChannels("client-1"), { wrapper });

		await waitFor(() => {
			expect(applyMemberCountChanged).toHaveBeenCalledWith("channel-1", 7);
		});
	});

	it("resets auth on client-not-found stream error", async () => {
		const resetAuth = vi.spyOn(useAuthStore.getState(), "reset");
		mocks.subscribeToChannelList.mockReturnValue({
			[Symbol.asyncIterator]() {
				return {
					next: () =>
						Promise.reject(new ConnectError("client not found", Code.NotFound)),
				};
			},
		});

		const { result } = renderHook(() => useChannels("client-1"), { wrapper });

		await waitFor(() => {
			expect(result.current.error).toBeInstanceOf(BroadcastError);
			expect(resetAuth).toHaveBeenCalled();
		});
	});

	it("resets channels on unmount", () => {
		const resetChannels = vi.spyOn(useChannelsStore.getState(), "reset");
		useChannelsStore
			.getState()
			.applySnapshot([{ id: "channel-1", name: "General", memberCount: 1 }]);
		const { unmount } = renderHook(() => useChannels("client-1"), { wrapper });

		unmount();

		expect(resetChannels).toHaveBeenCalled();
		expect(useChannelsStore.getState().channels.size).toBe(0);
		expect(useChannelsStore.getState().snapshotApplied).toBe(false);
	});

	it("passes the query abort signal to subscribeToChannelList", async () => {
		renderHook(() => useChannels("client-1"), { wrapper });

		await waitFor(() => {
			expect(subscribeToChannelList).toHaveBeenCalledWith("client-1", {
				signal: expect.any(AbortSignal),
			});
		});
	});
});
