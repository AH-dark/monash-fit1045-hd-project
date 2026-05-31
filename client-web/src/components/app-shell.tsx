import type { ReactNode } from "react";

import {
	SidebarInset,
	SidebarProvider,
	SidebarTrigger,
} from "@/components/ui/sidebar";
import { useHeartbeatScheduler } from "@/hooks/use-heartbeat-scheduler";

import { AppSidebar } from "./app-sidebar";

export function AppShell({ children }: { children: ReactNode }) {
	useHeartbeatScheduler();

	return (
		<SidebarProvider defaultOpen={true}>
			<AppSidebar />
			<SidebarInset className="h-svh overflow-hidden">
				<header className="flex h-16 shrink-0 items-center gap-2 border-b px-4">
					<SidebarTrigger className="-ml-1" />
				</header>
				<main className="flex min-h-0 flex-1 flex-col gap-4 p-4">
					{children}
				</main>
			</SidebarInset>
		</SidebarProvider>
	);
}
