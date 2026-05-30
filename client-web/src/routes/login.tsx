import { createFileRoute, redirect } from "@tanstack/react-router";

import { LoginPage } from "@/pages/login/login-page";
import { useAuthStore } from "@/stores/auth-store";

export const Route = createFileRoute("/login")({
	beforeLoad: () => {
		if (useAuthStore.getState().status === "connected") {
			throw redirect({ to: "/" as never });
		}
	},
	component: () => <LoginPage />,
});
