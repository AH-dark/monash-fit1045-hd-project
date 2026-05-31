import type {
	ChannelEvent,
	ChannelListEvent,
} from "@/gen/bcmd/v1/broadcast_pb.ts";

import { broadcastClient } from "./client.ts";

export async function connect(username: string): Promise<{ clientId: string }> {
	const res = await broadcastClient.connect({ username });
	return { clientId: res.clientId };
}

export async function disconnect(clientId: string): Promise<void> {
	await broadcastClient.disconnect({ clientId });
}

export async function listChannels(): Promise<
	Array<{ id: string; name: string; memberCount: number }>
> {
	const res = await broadcastClient.listChannels({});
	return res.channels.map((c) => ({
		id: c.id,
		name: c.name,
		memberCount: c.memberCount,
	}));
}

export async function createChannel(
	clientId: string,
	channelName: string,
): Promise<{ channelId: string; channelName: string }> {
	const res = await broadcastClient.createChannel({ clientId, channelName });
	return { channelId: res.channelId, channelName: res.channelName };
}

export async function joinChannel(
	clientId: string,
	channelId: string,
): Promise<void> {
	await broadcastClient.joinChannel({ clientId, channelId });
}

export async function leaveChannel(
	clientId: string,
	channelId: string,
): Promise<void> {
	await broadcastClient.leaveChannel({ clientId, channelId });
}

export async function sendMessage(
	clientId: string,
	channelId: string,
	content: string,
): Promise<{ messageId: string }> {
	const res = await broadcastClient.sendMessage({
		clientId,
		channelId,
		content,
	});
	return { messageId: res.messageId };
}

export async function heartbeat(clientId: string): Promise<void> {
	await broadcastClient.heartbeat({ clientId });
}

export function subscribeToChannel(
	clientId: string,
	channelId: string,
	options?: { signal?: AbortSignal },
): AsyncIterable<ChannelEvent> {
	return broadcastClient.subscribeToChannel(
		{ clientId, channelId },
		{ signal: options?.signal },
	);
}

export type HistoryMessage = {
	messageId: string;
	channelId: string;
	senderId: string;
	senderName: string;
	content: string;
	timestamp: number;
};

export type ListMessagesPage = {
	messages: HistoryMessage[];
	hasMore: boolean;
};

export async function listMessages(
	clientId: string,
	channelId: string,
	beforeMessageId: string | null,
	limit: number,
): Promise<ListMessagesPage> {
	const res = await broadcastClient.listMessages({
		clientId,
		channelId,
		beforeMessageId: beforeMessageId ?? "",
		limit,
	});
	return {
		messages: res.messages.map((m) => ({
			messageId: m.messageId,
			channelId: m.channelId,
			senderId: m.senderId,
			senderName: m.senderName,
			content: m.content,
			timestamp: Number(m.sentAtMs),
		})),
		hasMore: res.hasMore,
	};
}

export function subscribeToChannelList(
	clientId: string,
	options?: { signal?: AbortSignal },
): AsyncIterable<ChannelListEvent> {
	return broadcastClient.subscribeToChannelList(
		{ clientId },
		{ signal: options?.signal },
	);
}
