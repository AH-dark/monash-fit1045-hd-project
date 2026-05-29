import { createFileRoute, Outlet, redirect } from "@tanstack/react-router";

import { AppShell } from "@/components/app-shell";
import { useAuthStore } from "@/stores/auth-store";

export const Route = createFileRoute("/_authenticated")({
	beforeLoad: () => {
		if (useAuthStore.getState().status !== "connected") {
			throw redirect({ to: "/login" as never });
		}
	},
	component: () => (
		<AppShell>
			<Outlet />
		</AppShell>
	),
});
