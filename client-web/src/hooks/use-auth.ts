import {
	type UseMutationOptions,
	type UseMutationResult,
	useMutation,
} from "@tanstack/react-query";
import { useShallow } from "zustand/react/shallow";

import {
	type BroadcastError,
	isClientNotFound,
	mapError,
} from "@/api/broadcast/errors";
import {
	connect as rpcConnect,
	disconnect as rpcDisconnect,
} from "@/api/broadcast/operations";
import type { AuthState } from "@/schemas/auth";
import { useAuthStore } from "@/stores/auth-store";

type ConnectResponse = { clientId: string };

export function useConnectMutation(
	options?: UseMutationOptions<ConnectResponse, BroadcastError, string>,
): UseMutationResult<ConnectResponse, BroadcastError, string> {
	const { setConnecting, setConnected, setDisconnected, reset } = useAuthStore(
		useShallow((s) => ({
			setConnecting: s.setConnecting,
			setConnected: s.setConnected,
			setDisconnected: s.setDisconnected,
			reset: s.reset,
		})),
	);

	return useMutation({
		mutationFn: async (username: string) => {
			try {
				return await rpcConnect(username);
			} catch (err) {
				throw mapError(err);
			}
		},
		onMutate: () => {
			setConnecting();
		},
		onSuccess: ({ clientId }, username) => {
			setConnected(clientId, username);
		},
		onError: (err) => {
			if (isClientNotFound(err)) {
				reset();
			} else {
				setDisconnected();
			}
		},
		...options,
	});
}

export function useDisconnectMutation(
	options?: UseMutationOptions<void, BroadcastError, string>,
): UseMutationResult<void, BroadcastError, string> {
	const { setDisconnected, reset } = useAuthStore(
		useShallow((s) => ({
			setDisconnected: s.setDisconnected,
			reset: s.reset,
		})),
	);

	return useMutation({
		mutationFn: async (clientId: string) => {
			try {
				await rpcDisconnect(clientId);
			} catch (err) {
				throw mapError(err);
			}
		},
		onError: (err) => {
			if (isClientNotFound(err)) {
				reset();
			}
		},
		onSettled: () => {
			setDisconnected();
		},
		...options,
	});
}

type UseAuthResult = AuthState & {
	reset: () => void;
	connect: UseMutationResult<
		ConnectResponse,
		BroadcastError,
		string
	>["mutateAsync"];
	disconnect: UseMutationResult<void, BroadcastError, string>["mutateAsync"];
};

export function useAuth(): UseAuthResult {
	const state = useAuthStore(
		useShallow((s) => ({
			status: s.status,
			clientId: s.clientId,
			username: s.username,
			reset: s.reset,
		})),
	);
	const connectMutation = useConnectMutation();
	const disconnectMutation = useDisconnectMutation();

	return {
		status: state.status,
		clientId: state.clientId,
		username: state.username,
		reset: state.reset,
		connect: connectMutation.mutateAsync,
		disconnect: disconnectMutation.mutateAsync,
	};
}
