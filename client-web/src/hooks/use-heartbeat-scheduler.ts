import { useEffect } from "react";

import { useShallow } from "zustand/react/shallow";

import { env } from "@/env";
import { useHeartbeatMutation } from "@/hooks/use-heartbeat-mutation";
import { useAuthStore } from "@/stores/auth-store";

export function useHeartbeatScheduler(): void {
	const { status, clientId } = useAuthStore(
		useShallow((s) => ({ status: s.status, clientId: s.clientId })),
	);
	const heartbeat = useHeartbeatMutation();

	useEffect(() => {
		if (status !== "connected" || !clientId) return;

		const interval = setInterval(() => {
			heartbeat.mutate({ clientId });
		}, env.VITE_HEARTBEAT_INTERVAL_MS ?? 3000);

		return () => clearInterval(interval);
	}, [status, clientId, heartbeat.mutate]);
}
