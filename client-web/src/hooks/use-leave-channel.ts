import { useCallback } from "react";

import { mapError } from "@/api/broadcast/errors";
import { leaveChannel as rpcLeaveChannel } from "@/api/broadcast/operations";

export function useLeaveChannel() {
	const leaveChannel = useCallback(
		async (clientId: string, channelId: string) => {
			try {
				await rpcLeaveChannel(clientId, channelId);
			} catch (err) {
				throw mapError(err);
			}
		},
		[],
	);

	return { leaveChannel };
}
