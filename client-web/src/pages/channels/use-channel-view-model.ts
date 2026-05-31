import { useMemo } from "react";

import { useShallow } from "zustand/react/shallow";

import type { BroadcastError } from "@/api/broadcast/errors";
import { useChannelMessages } from "@/hooks/use-channel-messages";
import { useLeaveChannelMutation } from "@/hooks/use-leave-channel-mutation";
import { useSendMessageMutation } from "@/hooks/use-send-message";
import type { Message } from "@/schemas/message";
import { MessageContentSchema } from "@/schemas/message";
import { useAuthStore } from "@/stores/auth-store";
import { useChannelsStore } from "@/stores/channels-store";

export interface ChannelViewModel {
	readonly messages: Message[];
	readonly channelName: string | undefined;
	readonly memberCount: number | undefined;
	readonly isConnected: boolean;
	readonly error: BroadcastError | null;
	readonly hasMoreHistory: boolean;
	readonly isLoadingHistory: boolean;
	readonly loadOlderHistory: () => void;
	readonly sendMessage: (content: string) => Promise<void>;
	readonly leaveChannel: () => Promise<void>;
	readonly schema: typeof MessageContentSchema;
}

export function useChannelViewModel(channelId: string): ChannelViewModel {
	const clientId = useAuthStore(useShallow((s) => s.clientId));
	const channel = useChannelsStore(
		useShallow((s) => s.channels.get(channelId)),
	);
	const {
		messages,
		isConnected,
		error,
		hasMore,
		isLoadingInitial,
		isLoadingMore,
		fetchOlder,
	} = useChannelMessages({ clientId, channelId });
	const sendMutation = useSendMessageMutation();
	const leaveMutation = useLeaveChannelMutation();

	const sendMutateAsync = sendMutation.mutateAsync;
	const leaveMutateAsync = leaveMutation.mutateAsync;

	return useMemo<ChannelViewModel>(
		() => ({
			messages,
			channelName: channel?.name,
			memberCount: channel?.memberCount,
			isConnected,
			error,
			hasMoreHistory: hasMore,
			isLoadingHistory: isLoadingInitial || isLoadingMore,
			loadOlderHistory: fetchOlder,
			sendMessage: async (content: string) => {
				if (!clientId) throw new Error("Not connected");
				await sendMutateAsync({ clientId, channelId, content });
			},
			leaveChannel: async () => {
				if (!clientId) throw new Error("Not connected");
				await leaveMutateAsync({ clientId, channelId });
			},
			schema: MessageContentSchema,
		}),
		[
			messages,
			channel,
			isConnected,
			error,
			hasMore,
			isLoadingInitial,
			isLoadingMore,
			fetchOlder,
			clientId,
			channelId,
			sendMutateAsync,
			leaveMutateAsync,
		],
	);
}
