import { create } from "zustand";
import { createJSONStorage, persist } from "zustand/middleware";

import type { AuthState, AuthStatusValue as AuthStatus } from "@/schemas/auth";

type AuthActions = {
	setConnecting: () => void;
	setConnected: (clientId: string, username: string) => void;
	setDisconnected: () => void;
	reset: () => void;
};

const initialState = {
	status: "idle" as AuthStatus,
	clientId: null,
	username: null,
} satisfies AuthState;

export const useAuthStore = create<AuthState & AuthActions>()(
	persist(
		(set) => ({
			...initialState,
			setConnecting: () => set({ status: "connecting" }),
			setConnected: (clientId, username) =>
				set({ status: "connected", clientId, username }),
			setDisconnected: () =>
				set({ status: "disconnected", clientId: null, username: null }),
			reset: () =>
				set({ status: "disconnected", clientId: null, username: null }),
		}),
		{
			name: "bcmd-auth",
			storage: createJSONStorage(() => localStorage),
			partialize: (s) => ({
				username: s.username,
			}),
		},
	),
);
