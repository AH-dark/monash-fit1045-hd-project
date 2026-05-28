import { useCallback } from "react";

import { mapError } from "@/api/broadcast/errors";
import { sendMessage as rpcSendMessage } from "@/api/broadcast/operations";

export function useSendMessage() {
	const sendMessage = useCallback(
		async (clientId: string, channelId: string, content: string) => {
			try {
				return await rpcSendMessage(clientId, channelId, content);
			} catch (err) {
				throw mapError(err);
			}
		},
		[],
	);

	return { sendMessage };
}
