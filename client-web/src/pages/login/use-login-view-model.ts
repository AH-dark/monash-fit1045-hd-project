import { useMemo } from "react";

import type { BroadcastError } from "@/api/broadcast/errors";
import { useAuth } from "@/hooks/use-auth";
import { UsernameInputSchema } from "@/schemas/auth";

export interface LoginViewModel {
	readonly status: string;
	readonly isConnecting: boolean;
	readonly error: BroadcastError | null;
	readonly connect: (username: string) => Promise<{ clientId: string }>;
	readonly schema: typeof UsernameInputSchema;
}

export function useLoginViewModel(): LoginViewModel {
	const { status, connect } = useAuth();
	return useMemo(
		() => ({
			status,
			isConnecting: status === "connecting",
			error: null,
			connect,
			schema: UsernameInputSchema,
		}),
		[status, connect],
	);
}
