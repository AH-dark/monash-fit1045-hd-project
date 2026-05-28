import { create } from "zustand";

type Message = {
	messageId: string;
	channelId: string;
	senderId: string;
	senderName: string;
	content: string;
	timestamp: number;
};

type MessagesState = {
	messages: Map<string, Message[]>;
};

type MessagesActions = {
	addMessage: (message: Message) => void;
	setMessages: (channelId: string, messages: Message[]) => void;
	clearMessages: (channelId: string) => void;
	reset: () => void;
};

export const useMessagesStore = create<MessagesState & MessagesActions>()(
	(set) => ({
		messages: new Map(),
		addMessage: (message) =>
			set((state) => {
				const channelMsgs = state.messages.get(message.channelId) ?? [];
				return {
					messages: new Map(state.messages).set(message.channelId, [
						...channelMsgs,
						message,
					]),
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
