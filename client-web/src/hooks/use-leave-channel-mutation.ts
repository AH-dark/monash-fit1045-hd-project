import {
	type UseMutationOptions,
	type UseMutationResult,
	useMutation,
} from "@tanstack/react-query";

import { type BroadcastError, mapError } from "@/api/broadcast/errors";
import { leaveChannel } from "@/api/broadcast/operations";
import type { LeaveChannelVariables } from "@/schemas/channel";

export function useLeaveChannelMutation(
	options?: UseMutationOptions<void, BroadcastError, LeaveChannelVariables>,
): UseMutationResult<void, BroadcastError, LeaveChannelVariables> {
	return useMutation({
		mutationFn: async ({ clientId, channelId }) => {
			try {
				await leaveChannel(clientId, channelId);
			} catch (err) {
				throw mapError(err);
			}
		},
		...options,
	});
}
