import {
	type UseMutationOptions,
	type UseMutationResult,
	useMutation,
} from "@tanstack/react-query";

import { type BroadcastError, mapError } from "@/api/broadcast/errors";
import { createChannel } from "@/api/broadcast/operations";
import type { CreateChannelVariables } from "@/schemas/channel";

type CreateChannelResponse = { channelId: string; channelName: string };

export function useCreateChannelMutation(
	options?: UseMutationOptions<
		CreateChannelResponse,
		BroadcastError,
		CreateChannelVariables
	>,
): UseMutationResult<
	CreateChannelResponse,
	BroadcastError,
	CreateChannelVariables
> {
	return useMutation({
		mutationFn: async ({ clientId, channelName }) => {
			try {
				return await createChannel(clientId, channelName);
			} catch (err) {
				throw mapError(err);
			}
		},
		...options,
	});
}
