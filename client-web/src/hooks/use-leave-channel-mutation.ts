import { useMutation } from "@tanstack/react-query";

import { leaveChannel } from "@/api/broadcast/operations";

type LeaveChannelVariables = {
	clientId: string;
	channelId: string;
};

export const useLeaveChannelMutation = () =>
	useMutation({
		mutationFn: ({ clientId, channelId }: LeaveChannelVariables) =>
			leaveChannel(clientId, channelId),
	});
