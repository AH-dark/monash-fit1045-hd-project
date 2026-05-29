import { useMutation } from "@tanstack/react-query";

import { createChannel } from "@/api/broadcast/operations";

type CreateChannelVariables = {
	clientId: string;
	channelName: string;
};

export const useCreateChannelMutation = () =>
	useMutation({
		mutationFn: ({ clientId, channelName }: CreateChannelVariables) =>
			createChannel(clientId, channelName),
	});
