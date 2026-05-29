import { useMemo } from "react";
import { useShallow } from "zustand/react/shallow";

import { useDisconnectMutation } from "@/hooks/use-auth";
import { useAuthStore } from "@/stores/auth-store";

export interface DisconnectViewModel {
	readonly isDisconnecting: boolean;
	readonly disconnect: () => Promise<void>;
}

export function useDisconnectViewModel(): DisconnectViewModel {
	const clientId = useAuthStore(useShallow((s) => s.clientId));
	const mutation = useDisconnectMutation();

	return useMemo(
		() => ({
			isDisconnecting: mutation.isPending,
			disconnect: async () => {
				if (!clientId) throw new Error("Not connected");
				await mutation.mutateAsync(clientId);
			},
		}),
		[clientId, mutation.isPending, mutation.mutateAsync],
	);
}
