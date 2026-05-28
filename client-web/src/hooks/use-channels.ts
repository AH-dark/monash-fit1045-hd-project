import { useEffect } from "react";

import { useShallow } from "zustand/react/shallow";

import { isClientNotFound, mapError } from "@/api/broadcast/errors";
import { subscribeToChannelList } from "@/api/broadcast/operations";
import { useAuthStore } from "@/stores/auth-store";
import { useChannelsStore } from "@/stores/channels-store";

export function useChannels(clientId: string | null) {
	const {
		applySnapshot,
		applyCreated,
		applyMemberCountChanged,
		resetChannels,
		channels,
		snapshotApplied,
	} = useChannelsStore(
		useShallow((s) => ({
			applySnapshot: s.applySnapshot,
			applyCreated: s.applyCreated,
			applyMemberCountChanged: s.applyMemberCountChanged,
			resetChannels: s.reset,
			channels: s.channels,
			snapshotApplied: s.snapshotApplied,
		})),
	);
	const { resetAuth } = useAuthStore(
		useShallow((s) => ({ resetAuth: s.reset })),
	);

	useEffect(() => {
		if (!clientId) return;

		let cancelled = false;
		const currentClientId = clientId;

		async function subscribe() {
			const stream = subscribeToChannelList(currentClientId);
			try {
				for await (const event of stream) {
					if (cancelled) break;
					const e = event.event;
					if (!e) continue;
					switch (e.case) {
						case "snapshot":
							applySnapshot(
								e.value.channels.map((ch) => ({
									id: ch.id,
									name: ch.name,
									memberCount: ch.memberCount,
								})),
							);
							break;
						case "created":
							if (e.value.channel) {
								applyCreated({
									id: e.value.channel.id,
									name: e.value.channel.name,
									memberCount: e.value.channel.memberCount,
								});
							}
							break;
						case "memberCountChanged":
							applyMemberCountChanged(e.value.channelId, e.value.memberCount);
							break;
					}
				}
			} catch (err) {
				if (cancelled) return;
				const broadcastError = mapError(err);
				if (isClientNotFound(broadcastError)) {
					resetAuth();
				}
			}
		}

		subscribe();
		return () => {
			cancelled = true;
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

	return { channels, snapshotApplied };
}
