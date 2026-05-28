import { useCallback } from "react";
import { isClientNotFound, mapError } from "#/api/broadcast/errors";
import {
	connect as rpcConnect,
	disconnect as rpcDisconnect,
} from "#/api/broadcast/operations";
import { useAuthStore } from "#/stores/auth-store";

export function useAuth() {
	const status = useAuthStore((s) => s.status);
	const clientId = useAuthStore((s) => s.clientId);
	const username = useAuthStore((s) => s.username);
	const setConnecting = useAuthStore((s) => s.setConnecting);
	const setConnected = useAuthStore((s) => s.setConnected);
	const setDisconnected = useAuthStore((s) => s.setDisconnected);
	const reset = useAuthStore((s) => s.reset);

	const connect = useCallback(
		async (usernameInput: string) => {
			setConnecting();
			try {
				const result = await rpcConnect(usernameInput);
				setConnected(result.clientId, usernameInput);
				return result.clientId;
			} catch (err) {
				const broadcastError = mapError(err);
				if (isClientNotFound(broadcastError)) {
					reset();
				} else {
					setDisconnected();
				}
				throw broadcastError;
			}
		},
		[setConnecting, setConnected, setDisconnected, reset],
	);

	const disconnectUser = useCallback(async () => {
		if (!clientId) return;
		try {
			await rpcDisconnect(clientId);
		} catch (err) {
			const broadcastError = mapError(err);
			if (isClientNotFound(broadcastError)) {
				reset();
				return;
			}
			throw broadcastError;
		} finally {
			setDisconnected();
		}
	}, [clientId, setDisconnected, reset]);

	return {
		status,
		clientId,
		username,
		connect,
		disconnect: disconnectUser,
		reset,
	};
}
