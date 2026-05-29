import { useEffect } from "react";

import {
	experimental_streamedQuery as streamedQuery,
	useQuery,
} from "@tanstack/react-query";
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

async function* makeChannelListGenerator(
	args: { clientId: string; signal?: AbortSignal },
	dispatch: (event: ChannelListEvent) => void,
): AsyncGenerator<ChannelListEvent> {
	try {
		for await (const event of subscribeToChannelList(args.clientId, {
			signal: args.signal,
		})) {
			dispatch(event);
			yield event;
		}
	} catch (err) {
		throw mapError(err);
	}
}

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

	const query = useQuery({
		queryKey: ["channels", clientId],
		enabled: !!clientId && typeof window !== "undefined",
		staleTime: Number.POSITIVE_INFINITY,
		gcTime: 0,
		refetchOnWindowFocus: false,
		retry: (failureCount, err) => !isClientNotFound(err) && failureCount < 3,
		queryFn: streamedQuery({
			streamFn: ({ signal }) => {
				if (!clientId) {
					throw new Error("clientId is required");
				}
				return makeChannelListGenerator({ clientId, signal }, (event) => {
					processChannelListEvent(
						event,
						applySnapshot,
						applyCreated,
						applyMemberCountChanged,
					);
				});
			},
			reducer: (_: ChannelListEvent | null, chunk) => chunk,
			initialValue: null as ChannelListEvent | null,
			refetchMode: "reset",
		}),
	});

	useEffect(() => {
		const err = query.error;
		if (err && isClientNotFound(err)) {
			resetAuth();
		}
	}, [query.error, resetAuth]);

	useEffect(() => {
		return () => {
			resetChannels();
		};
	}, [resetChannels]);

	return {
		channels,
		snapshotApplied,
		error: query.error as BroadcastError | null,
	};
}
