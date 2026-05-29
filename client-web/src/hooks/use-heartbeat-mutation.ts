import {
	type UseMutationOptions,
	type UseMutationResult,
	useMutation,
} from "@tanstack/react-query";

import { type BroadcastError, mapError } from "@/api/broadcast/errors";
import { heartbeat } from "@/api/broadcast/operations";
import type { HeartbeatVariables } from "@/schemas/heartbeat";

export function useHeartbeatMutation(
	options?: UseMutationOptions<void, BroadcastError, HeartbeatVariables>,
): UseMutationResult<void, BroadcastError, HeartbeatVariables> {
	return useMutation({
		mutationFn: async ({ clientId }) => {
			try {
				await heartbeat(clientId);
			} catch (err) {
				throw mapError(err);
			}
		},
		...options,
	});
}
