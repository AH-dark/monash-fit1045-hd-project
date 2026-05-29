import {
	type UseMutationOptions,
	type UseMutationResult,
	useMutation,
} from "@tanstack/react-query";

import { type BroadcastError, mapError } from "@/api/broadcast/errors";
import { sendMessage } from "@/api/broadcast/operations";
import type { SendMessageVariables } from "@/schemas/message";

type SendMessageResponse = { messageId: string };

export function useSendMessageMutation(
	options?: UseMutationOptions<
		SendMessageResponse,
		BroadcastError,
		SendMessageVariables
	>,
): UseMutationResult<
	SendMessageResponse,
	BroadcastError,
	SendMessageVariables
> {
	return useMutation({
		mutationFn: async ({ clientId, channelId, content }) => {
			try {
				return await sendMessage(clientId, channelId, content);
			} catch (err) {
				throw mapError(err);
			}
		},
		...options,
	});
}
