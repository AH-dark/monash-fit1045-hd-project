import { create } from "zustand";

type Channel = {
	id: string;
	name: string;
	memberCount: number;
};

type ChannelsState = {
	channels: Map<string, Channel>;
	activeChannelId: string | null;
	snapshotApplied: boolean;
};

type ChannelsActions = {
	applySnapshot: (channels: ReadonlyArray<Channel>) => void;
	applyCreated: (channel: Channel) => void;
	applyMemberCountChanged: (channelId: string, memberCount: number) => void;
	setActiveChannel: (channelId: string | null) => void;
	reset: () => void;
};

const initialState: ChannelsState = {
	channels: new Map(),
	activeChannelId: null,
	snapshotApplied: false,
};

export const useChannelsStore = create<ChannelsState & ChannelsActions>()(
	(set) => ({
		...initialState,
		applySnapshot: (channels) => {
			const map = new Map<string, Channel>();
			for (const ch of channels) {
				map.set(ch.id, ch);
			}
			set({ channels: map, snapshotApplied: true });
		},
		applyCreated: (channel) =>
			set((state) => ({
				channels: new Map(state.channels).set(channel.id, channel),
			})),
		applyMemberCountChanged: (channelId, memberCount) =>
			set((state) => {
				const existing = state.channels.get(channelId);
				if (!existing) return state;
				return {
					channels: new Map(state.channels).set(channelId, {
						...existing,
						memberCount,
					}),
				};
			}),
		setActiveChannel: (channelId) => set({ activeChannelId: channelId }),
		reset: () => set({ ...initialState, channels: new Map() }),
	}),
);
