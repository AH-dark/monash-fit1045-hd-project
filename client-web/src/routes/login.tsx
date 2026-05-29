import { createFileRoute, redirect } from "@tanstack/react-router";

import { useAuthStore } from "@/stores/auth-store";

import { LoginPage } from "@/pages/login/login-page";

export const Route = createFileRoute("/login")({
	beforeLoad: () => {
		if (useAuthStore.getState().status === "connected") {
			throw redirect({ to: "/" as never });
		}
	},
	component: () => <LoginPage />,
});
