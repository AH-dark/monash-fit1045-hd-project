import { create } from "zustand";
import { createJSONStorage, persist } from "zustand/middleware";

type AuthStatus = "idle" | "connecting" | "connected" | "disconnected";

type AuthState = {
	status: AuthStatus;
	clientId: string | null;
	username: string | null;
};

type AuthActions = {
	setConnecting: () => void;
	setConnected: (clientId: string, username: string) => void;
	setDisconnected: () => void;
	reset: () => void;
};

const initialState: AuthState = {
	status: "idle",
	clientId: null,
	username: null,
};

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
			partialize: (s) => ({ clientId: s.clientId, username: s.username }),
		},
	),
);
