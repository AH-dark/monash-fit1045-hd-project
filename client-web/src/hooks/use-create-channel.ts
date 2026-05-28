import { useCallback } from "react";
import { mapError } from "#/api/broadcast/errors";
import { createChannel as rpcCreateChannel } from "#/api/broadcast/operations";
import { useChannelsStore } from "#/stores/channels-store";

export function useCreateChannel() {
	const applyCreated = useChannelsStore((s) => s.applyCreated);

	const createChannel = useCallback(
		async (clientId: string, channelName: string) => {
			try {
				const result = await rpcCreateChannel(clientId, channelName);
				applyCreated({
					id: result.channelId,
					name: result.channelName,
					memberCount: 1,
				});
				return result;
			} catch (err) {
				throw mapError(err);
			}
		},
		[applyCreated],
	);

	return { createChannel };
}
