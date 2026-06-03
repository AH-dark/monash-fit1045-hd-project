import { useEffect, useMemo, useRef, useState } from "react";

import {
	type InfiniteData,
	type QueryClient,
	useInfiniteQuery,
	useQueryClient,
} from "@tanstack/react-query";

import {
	type BroadcastError,
	isClientNotFound,
	mapError,
} from "@/api/broadcast/errors";
import {
	joinChannel,
	type ListMessagesPage,
	listMessages,
	subscribeToChannel,
} from "@/api/broadcast/operations";
import { env } from "@/env";
import type { Message } from "@/schemas/message";
import { useAuthStore } from "@/stores/auth-store";

type UseChannelMessagesArgs = {
	clientId: string | null;
	channelId: string;
};

type ConnectionState = "idle" | "joining" | "connected" | "disconnected";

type HistoryQueryData = InfiniteData<ListMessagesPage, string | null>;

export const channelMessagesKeys = {
	history: (clientId: string | null, channelId: string) =>
		["channel-history", clientId, channelId] as const,
};

function injectLiveMessage(
	queryClient: QueryClient,
	clientId: string,
	channelId: string,
	message: Message,
): void {
	queryClient.setQueryData<HistoryQueryData>(
		channelMessagesKeys.history(clientId, channelId),
		(old) => {
			if (!old) {
				return {
					pages: [{ messages: [message], hasMore: false }],
					pageParams: [null],
				};
			}
			const exists = old.pages.some((page) =>
				page.messages.some((m) => m.messageId === message.messageId),
			);
			if (exists) return old;
			const [first, ...rest] = old.pages;
			return {
				...old,
				pages: [{ ...first, messages: [...first.messages, message] }, ...rest],
			};
		},
	);
}

export function useChannelMessages({
	clientId,
	channelId,
}: UseChannelMessagesArgs) {
	const queryClient = useQueryClient();
	const [connectionState, setConnectionState] =
		useState<ConnectionState>("idle");
	const [isJoined, setIsJoined] = useState(false);
	const [streamError, setStreamError] = useState<BroadcastError | null>(null);
	const resetAuth = useAuthStore((s) => s.reset);

	const query = useInfiniteQuery<
		ListMessagesPage,
		BroadcastError,
		HistoryQueryData,
		ReturnType<typeof channelMessagesKeys.history>,
		string | null
	>({
		queryKey: channelMessagesKeys.history(clientId, channelId),
		enabled: Boolean(
			clientId && channelId && typeof window !== "undefined" && isJoined,
		),
		initialPageParam: null,
		staleTime: Number.POSITIVE_INFINITY,
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

	const pendingLiveRef = useRef<Message[]>([]);
	const initialFetchedRef = useRef(false);

	useEffect(() => {
		if (!query.isSuccess || initialFetchedRef.current) return;
		if (!clientId) return;
		initialFetchedRef.current = true;
		const drained = pendingLiveRef.current;
		pendingLiveRef.current = [];
		for (const msg of drained) {
			injectLiveMessage(queryClient, clientId, channelId, msg);
		}
	}, [query.isSuccess, queryClient, clientId, channelId]);

	useEffect(() => {
		pendingLiveRef.current = [];
		initialFetchedRef.current = false;
		setIsJoined(false);
		if (!clientId || !channelId || typeof window === "undefined") return;

		const abortController = new AbortController();
		const { signal } = abortController;

		setConnectionState("joining");
		setStreamError(null);

		void (async () => {
			try {
				await joinChannel(clientId, channelId);
				if (signal.aborted) return;
				setIsJoined(true);
				setConnectionState("connected");

				for await (const evt of subscribeToChannel(clientId, channelId, {
					signal,
				})) {
					if (signal.aborted) break;
					if (evt.event.case !== "message") continue;
					const m = evt.event.value;
					const message: Message = {
						messageId: m.messageId,
						channelId,
						senderId: m.senderId,
						senderName: m.senderName,
						content: m.content,
						timestamp: Number(m.sentAtMs),
					};
					if (initialFetchedRef.current) {
						injectLiveMessage(queryClient, clientId, channelId, message);
					} else {
						pendingLiveRef.current.push(message);
					}
				}
				if (!signal.aborted) {
					setConnectionState("disconnected");
				}
			} catch (err) {
				if (signal.aborted) return;
				const broadcastError = mapError(err);
				setStreamError(broadcastError);
				setConnectionState("disconnected");
				if (isClientNotFound(broadcastError)) {
					resetAuth();
				}
			}
		})();

		return () => {
			abortController.abort();
		};
	}, [clientId, channelId, queryClient, resetAuth]);

	const messages = useMemo<Message[]>(() => {
		const pages = query.data?.pages;
		if (!pages || pages.length === 0) return [];
		const byId = new Map<string, Message>();
		for (const page of pages) {
			for (const m of page.messages) {
				if (byId.has(m.messageId)) continue;
				byId.set(m.messageId, {
					messageId: m.messageId,
					channelId,
					senderId: m.senderId,
					senderName: m.senderName,
					content: m.content,
					timestamp: m.timestamp,
				});
			}
		}
		return Array.from(byId.values()).sort((a, b) => a.timestamp - b.timestamp);
	}, [query.data, channelId]);

	return {
		messages,
		isConnected: connectionState === "connected",
		error: streamError ?? (query.error as BroadcastError | null),
		hasMore: query.hasNextPage ?? false,
		isLoadingInitial: query.isPending,
		isLoadingMore: query.isFetchingNextPage,
		fetchOlder: () => {
			if (query.hasNextPage && !query.isFetchingNextPage) {
				void query.fetchNextPage();
			}
		},
	};
}
