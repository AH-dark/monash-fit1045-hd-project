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
import { joinChannel, subscribeToChannel } from "@/api/broadcast/operations";
import type { ChannelEvent } from "@/gen/bcmd/v1/broadcast_pb.ts";
import type { Message } from "@/schemas/message";
import { useAuthStore } from "@/stores/auth-store";
import { useMessagesStore } from "@/stores/messages-store";

type UseChannelArgs = {
	clientId: string | null;
	channelId: string;
	replayCount?: number;
};

type ChannelGeneratorArgs = {
	clientId: string;
	channelId: string;
	replayCount?: number;
	signal?: AbortSignal;
};

const emptyMessages: Message[] = [];

async function* makeChannelGenerator(
	args: ChannelGeneratorArgs,
	dispatch: (evt: ChannelEvent) => void,
	onClear: () => void,
): AsyncGenerator<ChannelEvent> {
	onClear();

	try {
		await joinChannel(args.clientId, args.channelId);
		for await (const evt of subscribeToChannel(
			args.clientId,
			args.channelId,
			args.replayCount,
			{ signal: args.signal },
		)) {
			dispatch(evt);
			yield evt;
		}
	} catch (err) {
		throw mapError(err);
	}
}

function processChannelEvent(
	evt: ChannelEvent,
	channelId: string,
	addMessage: (message: Message) => void,
): void {
	if (evt.event.case === "message") {
		const message = evt.event.value;
		addMessage({
			messageId: message.messageId,
			channelId,
			senderId: message.senderId,
			senderName: message.senderName,
			content: message.content,
			timestamp: Number(message.sentAtMs),
		});
	}
}

export function useChannel({
	clientId,
	channelId,
	replayCount,
}: UseChannelArgs) {
	const { messages, addMessage, clearMessages } = useMessagesStore(
		useShallow((s) => ({
			messages: s.messages.get(channelId) ?? emptyMessages,
			addMessage: s.addMessage,
			clearMessages: s.clearMessages,
		})),
	);
	const resetAuth = useAuthStore((s) => s.reset);

	const query = useQuery({
		queryKey: ["channel", clientId, channelId, replayCount],
		enabled: Boolean(clientId && channelId && typeof window !== "undefined"),
		staleTime: Number.POSITIVE_INFINITY,
		gcTime: 0,
		refetchOnWindowFocus: false,
		retry: (failureCount, err) => !isClientNotFound(err) && failureCount < 3,
		queryFn: streamedQuery<ChannelEvent, ChannelEvent | null>({
			streamFn: ({ signal }) => {
				if (!clientId) {
					throw new Error("clientId is required");
				}
				return makeChannelGenerator(
					{ clientId, channelId, replayCount, signal },
					(evt) => processChannelEvent(evt, channelId, addMessage),
					() => clearMessages(channelId),
				);
			},
			reducer: (_latest, chunk) => chunk,
			initialValue: null,
			refetchMode: "reset",
		}),
	});

	useEffect(() => {
		const err = query.error;
		if (err && isClientNotFound(err)) {
			resetAuth();
		}
	}, [query.error, resetAuth]);

	return {
		messages,
		isConnected: query.fetchStatus === "fetching",
		error: query.error as BroadcastError | null,
	};
}
