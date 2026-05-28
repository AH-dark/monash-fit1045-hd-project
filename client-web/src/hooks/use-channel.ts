import { useCallback, useEffect, useRef, useState } from "react";

import { useShallow } from "zustand/react/shallow";

import { isClientNotFound, mapError } from "@/api/broadcast/errors";
import {
	joinChannel,
	leaveChannel,
	subscribeToChannel,
} from "@/api/broadcast/operations";
import { useAuthStore } from "@/stores/auth-store";
import { useMessagesStore } from "@/stores/messages-store";

export function useChannel(clientId: string | null, channelId: string | null) {
	const [isConnected, setIsConnected] = useState(false);
	const { resetAuth } = useAuthStore(
		useShallow((s) => ({ resetAuth: s.reset })),
	);
	const { addMessage, clearMessages } = useMessagesStore(
		useShallow((s) => ({
			addMessage: s.addMessage,
			clearMessages: s.clearMessages,
		})),
	);
	const messages = useMessagesStore((s) =>
		channelId ? (s.messages.get(channelId) ?? []) : [],
	);

	const cancelledRef = useRef(false);

	const joinAndSubscribe = useCallback(async () => {
		if (!clientId || !channelId) return;

		try {
			await joinChannel(clientId, channelId);
			setIsConnected(true);
		} catch (err) {
			const broadcastError = mapError(err);
			if (isClientNotFound(broadcastError)) {
				resetAuth();
			}
			return;
		}

		const stream = subscribeToChannel(clientId, channelId);
		try {
			for await (const event of stream) {
				if (cancelledRef.current) break;
				const e = event.event;
				if (!e) continue;
				if (e.case === "message" && e.value) {
					addMessage({
						messageId: e.value.messageId,
						channelId,
						senderId: e.value.senderId,
						senderName: e.value.senderName,
						content: e.value.content,
						timestamp: Number(e.value.sentAtMs),
					});
				}
			}
		} catch (err) {
			if (cancelledRef.current) return;
			const broadcastError = mapError(err);
			if (isClientNotFound(broadcastError)) {
				resetAuth();
			}
		} finally {
			setIsConnected(false);
		}
	}, [clientId, channelId, resetAuth, addMessage]);

	useEffect(() => {
		if (!clientId || !channelId) return;

		cancelledRef.current = false;
		joinAndSubscribe();

		return () => {
			cancelledRef.current = true;
			clearMessages(channelId);
			leaveChannel(clientId, channelId).catch(() => {
				// best-effort leave on unmount
			});
			setIsConnected(false);
		};
	}, [clientId, channelId, joinAndSubscribe, clearMessages]);

	return { messages, isConnected };
}
