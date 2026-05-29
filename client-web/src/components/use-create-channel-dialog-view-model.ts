import { useMemo } from "react";

import * as z from "zod";
import { useShallow } from "zustand/react/shallow";

import type { BroadcastError } from "@/api/broadcast/errors";
import { useCreateChannelMutation } from "@/hooks/use-create-channel-mutation";
import { useAuthStore } from "@/stores/auth-store";

export type { BroadcastError };

export const ChannelNameSchema = z.object({
	channelName: z
		.string()
		.trim()
		.min(1, "Channel name is required")
		.max(50, "Channel name must be 50 characters or fewer")
		.regex(
			/^[a-zA-Z0-9 _-]+$/,
			"Only letters, numbers, spaces, _ and - allowed",
		),
});

export interface CreateChannelDialogViewModel {
	readonly schema: typeof ChannelNameSchema;
	readonly isCreating: boolean;
	readonly createChannel: (
		channelName: string,
	) => Promise<{ channelId: string; channelName: string }>;
}

export function useCreateChannelDialogViewModel(): CreateChannelDialogViewModel {
	const clientId = useAuthStore(useShallow((s) => s.clientId));
	const mutation = useCreateChannelMutation();

	return useMemo(
		() => ({
			schema: ChannelNameSchema,
			isCreating: mutation.isPending,
			createChannel: async (channelName: string) => {
				if (!clientId) {
					throw new Error("Not connected");
				}
				return mutation.mutateAsync({ clientId, channelName });
			},
		}),
		[clientId, mutation.isPending, mutation.mutateAsync],
	);
}
