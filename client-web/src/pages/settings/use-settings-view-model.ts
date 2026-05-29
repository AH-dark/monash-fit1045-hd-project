import { useMemo } from "react";

import { env } from "@/env";
import { useAuth } from "@/hooks/use-auth";

export interface SettingsViewModel {
	readonly username: string | null;
	readonly clientId: string | null;
	readonly heartbeatIntervalMs: number;
	readonly disconnect: () => Promise<void>;
}

export function useSettingsViewModel(): SettingsViewModel {
	const { username, clientId, disconnect } = useAuth();
	return useMemo(
		() => ({
			username,
			clientId,
			heartbeatIntervalMs: env.VITE_HEARTBEAT_INTERVAL_MS,
			disconnect: async () => {
				if (!clientId) {
					throw new Error("Not connected");
				}
				await disconnect(clientId);
			},
		}),
		[username, clientId, disconnect],
	);
}
