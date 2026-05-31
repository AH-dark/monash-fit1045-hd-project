import { useEffect, useState } from "react";

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
};

type ConnectionState = "idle" | "joining" | "connected" | "disconnected";

const emptyMessages: Message[] = [];

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

// Plain effect (not streamedQuery): the stream is long-lived, fetchStatus is not
// a reliable connected-state signal, and gcTime:0 + StrictMode caused rejoin storms.
export function useChannel({ clientId, channelId }: UseChannelArgs) {
	const [connectionState, setConnectionState] =
		useState<ConnectionState>("idle");
	const [error, setError] = useState<BroadcastError | null>(null);

	const { messages, addMessage, clearMessages } = useMessagesStore(
		useShallow((s) => ({
			messages: s.messages.get(channelId) ?? emptyMessages,
			addMessage: s.addMessage,
			clearMessages: s.clearMessages,
		})),
	);
	const resetAuth = useAuthStore((s) => s.reset);

	useEffect(() => {
		if (!clientId || !channelId || typeof window === "undefined") return;

		const abortController = new AbortController();
		const { signal } = abortController;

		setConnectionState("joining");
		setError(null);
		clearMessages(channelId);

		void (async () => {
			try {
				await joinChannel(clientId, channelId);
				if (signal.aborted) return;
				setConnectionState("connected");

				for await (const evt of subscribeToChannel(clientId, channelId, {
					signal,
				})) {
					if (signal.aborted) break;
					processChannelEvent(evt, channelId, addMessage);
				}
				if (!signal.aborted) {
					setConnectionState("disconnected");
				}
			} catch (err) {
				if (signal.aborted) return;
				const broadcastError = mapError(err);
				setError(broadcastError);
				setConnectionState("disconnected");
				if (isClientNotFound(broadcastError)) {
					resetAuth();
				}
			}
		})();

		return () => {
			abortController.abort();
		};
	}, [clientId, channelId, addMessage, clearMessages, resetAuth]);

	return {
		messages,
		isConnected: connectionState === "connected",
		error,
	};
}
