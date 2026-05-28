import { beforeEach, describe, expect, it } from "vitest";
import { useChannelsStore } from "#/stores/channels-store";

describe("channels-store", () => {
	beforeEach(() => {
		useChannelsStore.getState().reset();
	});

	it("applySnapshot replaces all channels", () => {
		useChannelsStore.getState().applyCreated({
			id: "stale",
			name: "stale",
			memberCount: 1,
		});
		useChannelsStore.getState().applySnapshot([
			{ id: "c1", name: "general", memberCount: 1 },
			{ id: "c2", name: "random", memberCount: 2 },
		]);
		const state = useChannelsStore.getState();
		expect(state.channels.size).toBe(2);
		expect(state.channels.has("stale")).toBe(false);
		expect(state.channels.get("c1")?.name).toBe("general");
		expect(state.channels.get("c2")?.memberCount).toBe(2);
		expect(state.snapshotApplied).toBe(true);
	});

	it("applyCreated adds a new channel", () => {
		useChannelsStore.getState().applyCreated({
			id: "c1",
			name: "general",
			memberCount: 1,
		});
		expect(useChannelsStore.getState().channels.get("c1")).toEqual({
			id: "c1",
			name: "general",
			memberCount: 1,
		});
	});

	it("applyMemberCountChanged updates member count for existing channel", () => {
		useChannelsStore.getState().applyCreated({
			id: "c1",
			name: "general",
			memberCount: 1,
		});
		useChannelsStore.getState().applyMemberCountChanged("c1", 5);
		expect(useChannelsStore.getState().channels.get("c1")?.memberCount).toBe(5);
	});

	it("applyMemberCountChanged is no-op for unknown channel", () => {
		useChannelsStore.getState().applyMemberCountChanged("unknown", 10);
		const state = useChannelsStore.getState();
		expect(state.channels.has("unknown")).toBe(false);
		expect(state.channels.size).toBe(0);
	});

	it("reset clears channels and activeChannelId", () => {
		useChannelsStore.getState().applyCreated({
			id: "c1",
			name: "general",
			memberCount: 1,
		});
		useChannelsStore.getState().setActiveChannel("c1");
		useChannelsStore.getState().reset();
		const state = useChannelsStore.getState();
		expect(state.channels.size).toBe(0);
		expect(state.activeChannelId).toBeNull();
		expect(state.snapshotApplied).toBe(false);
	});
});
