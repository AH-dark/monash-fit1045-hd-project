import { useMemo } from "react";

import { useAuth } from "@/hooks/use-auth";

export interface UserMenuViewModel {
	readonly username: string | null;
	readonly initial: string;
	readonly disconnect: () => Promise<void>;
}

export function useUserMenuViewModel(): UserMenuViewModel {
	const { username, clientId, disconnect } = useAuth();
	return useMemo(
		() => ({
			username,
			initial: username?.[0]?.toUpperCase() ?? "?",
			disconnect: async () => {
				if (!clientId) throw new Error("Not connected");
				await disconnect(clientId);
			},
		}),
		[username, clientId, disconnect],
	);
}
