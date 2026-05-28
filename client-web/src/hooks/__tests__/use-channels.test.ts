import { renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { useChannels } from "#/hooks/use-channels";
import { useChannelsStore } from "#/stores/channels-store";

vi.mock("#/api/broadcast/operations", () => ({
	subscribeToChannelList: vi.fn().mockReturnValue({
		[Symbol.asyncIterator]: () => ({ next: () => new Promise(() => {}) }),
	}),
}));

describe("useChannels", () => {
	beforeEach(() => {
		useChannelsStore.getState().reset();
	});

	it("returns channels and snapshotApplied from store", () => {
		const { result } = renderHook(() => useChannels("client-1"));

		expect(result.current.channels).toBeInstanceOf(Map);
		expect(result.current.snapshotApplied).toBe(false);
	});

	it("resets channels on unmount", () => {
		useChannelsStore
			.getState()
			.applySnapshot([{ id: "channel-1", name: "General", memberCount: 1 }]);
		const { unmount } = renderHook(() => useChannels("client-1"));

		unmount();

		expect(useChannelsStore.getState().channels.size).toBe(0);
		expect(useChannelsStore.getState().snapshotApplied).toBe(false);
	});
});
