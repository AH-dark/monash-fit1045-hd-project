import { useEffect, useState } from "react";

import { useShallow } from "zustand/react/shallow";

import {
	type BroadcastError,
	isClientNotFound,
	mapError,
} from "@/api/broadcast/errors";
import { subscribeToChannelList } from "@/api/broadcast/operations";
import type { ChannelListEvent } from "@/gen/bcmd/v1/broadcast_pb.ts";
import type { Channel } from "@/schemas/channel";
import { useAuthStore } from "@/stores/auth-store";
import { useChannelsStore } from "@/stores/channels-store";

function processChannelListEvent(
	event: ChannelListEvent,
	applySnapshot: (channels: ReadonlyArray<Channel>) => void,
	applyCreated: (channel: Channel) => void,
	applyMemberCountChanged: (channelId: string, memberCount: number) => void,
): void {
	switch (event.event.case) {
		case "snapshot":
			applySnapshot(
				event.event.value.channels.map((channel) => ({
					id: channel.id,
					name: channel.name,
					memberCount: channel.memberCount,
				})),
			);
			break;
		case "created":
			if (event.event.value.channel) {
				applyCreated({
					id: event.event.value.channel.id,
					name: event.event.value.channel.name,
					memberCount: event.event.value.channel.memberCount,
				});
			}
			break;
		case "memberCountChanged":
			applyMemberCountChanged(
				event.event.value.channelId,
				event.event.value.memberCount,
			);
			break;
	}
}

export function useChannels(clientId: string | null) {
	const [error, setError] = useState<BroadcastError | null>(null);

	const {
		applySnapshot,
		applyCreated,
		applyMemberCountChanged,
		resetChannels,
		channels,
		snapshotApplied,
	} = useChannelsStore(
		useShallow((state) => ({
			applySnapshot: state.applySnapshot,
			applyCreated: state.applyCreated,
			applyMemberCountChanged: state.applyMemberCountChanged,
			resetChannels: state.reset,
			channels: state.channels,
			snapshotApplied: state.snapshotApplied,
		})),
	);
	const resetAuth = useAuthStore((state) => state.reset);

	useEffect(() => {
		if (!clientId || typeof window === "undefined") return;

		const abortController = new AbortController();
		const { signal } = abortController;

		setError(null);

		void (async () => {
			try {
				for await (const event of subscribeToChannelList(clientId, {
					signal,
				})) {
					if (signal.aborted) break;
					processChannelListEvent(
						event,
						applySnapshot,
						applyCreated,
						applyMemberCountChanged,
					);
				}
			} catch (err) {
				if (signal.aborted) return;
				const broadcastError = mapError(err);
				setError(broadcastError);
				if (isClientNotFound(broadcastError)) {
					resetAuth();
				}
			}
		})();

		return () => {
			abortController.abort();
			resetChannels();
		};
	}, [
		clientId,
		applySnapshot,
		applyCreated,
		applyMemberCountChanged,
		resetChannels,
		resetAuth,
	]);

	return {
		channels,
		snapshotApplied,
		error,
	};
}
