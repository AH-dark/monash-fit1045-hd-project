import { useEffect } from "react";

import { useShallow } from "zustand/react/shallow";

import { isClientNotFound } from "@/api/broadcast/errors";
import { env } from "@/env";
import { useHeartbeatMutation } from "@/hooks/use-heartbeat-mutation";
import { useAuthStore } from "@/stores/auth-store";

export function useHeartbeatScheduler(): void {
	const { status, clientId, reset } = useAuthStore(
		useShallow((s) => ({
			status: s.status,
			clientId: s.clientId,
			reset: s.reset,
		})),
	);
	const heartbeat = useHeartbeatMutation();

	useEffect(() => {
		if (status !== "connected" || !clientId) return;

		const interval = setInterval(() => {
			heartbeat.mutate(
				{ clientId },
				{
					onError: (err) => {
						if (isClientNotFound(err)) {
							reset();
						}
					},
				},
			);
		}, env.VITE_HEARTBEAT_INTERVAL_MS ?? 3000);

		return () => clearInterval(interval);
	}, [status, clientId, heartbeat.mutate, reset]);
}
