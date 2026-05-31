import { createFileRoute } from "@tanstack/react-router";

import { EmptyDashboardPage } from "@/pages/dashboard/empty-dashboard-page";

export const Route = createFileRoute("/_authenticated/")({
	component: () => <EmptyDashboardPage />,
});
