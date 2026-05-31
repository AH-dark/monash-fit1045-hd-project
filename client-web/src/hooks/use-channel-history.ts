import { useEffect } from "react";

import { useInfiniteQuery } from "@tanstack/react-query";
import { useShallow } from "zustand/react/shallow";

import {
	type BroadcastError,
	isClientNotFound,
	mapError,
} from "@/api/broadcast/errors";
import {
	type ListMessagesPage,
	listMessages,
} from "@/api/broadcast/operations";
import { env } from "@/env";
import type { Message } from "@/schemas/message";
import { useAuthStore } from "@/stores/auth-store";
import { useMessagesStore } from "@/stores/messages-store";

type UseChannelHistoryArgs = {
	clientId: string | null;
	channelId: string;
};

export function useChannelHistory({
	clientId,
	channelId,
}: UseChannelHistoryArgs) {
	const prependMessages = useMessagesStore(
		useShallow((s) => s.prependMessages),
	);
	const resetAuth = useAuthStore((s) => s.reset);

	const query = useInfiniteQuery<
		ListMessagesPage,
		BroadcastError,
		{ pages: ListMessagesPage[]; pageParams: Array<string | null> },
		readonly [string, string | null, string],
		string | null
	>({
		queryKey: ["channel-history", clientId, channelId] as const,
		enabled: Boolean(clientId && channelId && typeof window !== "undefined"),
		initialPageParam: null,
		staleTime: 30_000,
		gcTime: 5 * 60_000,
		refetchOnWindowFocus: false,
		retry: (failureCount, err) => !isClientNotFound(err) && failureCount < 3,
		queryFn: async ({ pageParam }) => {
			if (!clientId) throw new Error("clientId is required");
			try {
				return await listMessages(
					clientId,
					channelId,
					pageParam,
					env.VITE_CHANNEL_HISTORY_PAGE_SIZE,
				);
			} catch (err) {
				throw mapError(err);
			}
		},
		getNextPageParam: (lastPage) =>
			lastPage.hasMore && lastPage.messages.length > 0
				? lastPage.messages[0].messageId
				: undefined,
	});

	useEffect(() => {
		if (query.error && isClientNotFound(query.error)) {
			resetAuth();
		}
	}, [query.error, resetAuth]);

	useEffect(() => {
		if (!query.data) return;
		const latestPage = query.data.pages[query.data.pages.length - 1];
		if (!latestPage) return;
		const messages: Message[] = latestPage.messages.map((m) => ({
			messageId: m.messageId,
			channelId: m.channelId,
			senderId: m.senderId,
			senderName: m.senderName,
			content: m.content,
			timestamp: m.timestamp,
		}));
		if (messages.length > 0) {
			prependMessages(channelId, messages);
		}
	}, [query.data, channelId, prependMessages]);

	return {
		isLoadingInitial: query.isPending,
		isLoadingMore: query.isFetchingNextPage,
		hasMore: query.hasNextPage ?? false,
		error: query.error,
		fetchOlder: () => {
			if (query.hasNextPage && !query.isFetchingNextPage) {
				void query.fetchNextPage();
			}
		},
	};
}
