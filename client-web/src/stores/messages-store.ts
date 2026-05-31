import { create } from "zustand";

import type { Message } from "@/schemas/message";

type MessagesState = {
	messages: Map<string, Message[]>;
};

type MessagesActions = {
	addMessage: (message: Message) => void;
	prependMessages: (channelId: string, messages: Message[]) => void;
	setMessages: (channelId: string, messages: Message[]) => void;
	clearMessages: (channelId: string) => void;
	reset: () => void;
};

function mergeAndSort(existing: Message[], incoming: Message[]): Message[] {
	const byId = new Map<string, Message>();
	for (const message of existing) {
		byId.set(message.messageId, message);
	}
	for (const message of incoming) {
		byId.set(message.messageId, message);
	}
	return Array.from(byId.values()).sort((a, b) => a.timestamp - b.timestamp);
}

export const useMessagesStore = create<MessagesState & MessagesActions>()(
	(set) => ({
		messages: new Map(),
		addMessage: (message) =>
			set((state) => {
				const channelMsgs = state.messages.get(message.channelId) ?? [];
				return {
					messages: new Map(state.messages).set(
						message.channelId,
						mergeAndSort(channelMsgs, [message]),
					),
				};
			}),
		prependMessages: (channelId, incoming) =>
			set((state) => {
				if (incoming.length === 0) return state;
				const channelMsgs = state.messages.get(channelId) ?? [];
				return {
					messages: new Map(state.messages).set(
						channelId,
						mergeAndSort(channelMsgs, incoming),
					),
				};
			}),
		setMessages: (channelId, messages) =>
			set((state) => ({
				messages: new Map(state.messages).set(channelId, messages),
			})),
		clearMessages: (channelId) =>
			set((state) => {
				const next = new Map(state.messages);
				next.delete(channelId);
				return { messages: next };
			}),
		reset: () => set({ messages: new Map() }),
	}),
);
