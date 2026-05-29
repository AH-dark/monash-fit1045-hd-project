import { useMutation } from "@tanstack/react-query";

import { heartbeat } from "@/api/broadcast/operations";

export const useHeartbeatMutation = () =>
	useMutation({
		mutationFn: (clientId: string) => heartbeat(clientId),
	});
