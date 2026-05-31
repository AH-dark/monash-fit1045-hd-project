import { useState } from "react";

import { Link } from "@tanstack/react-router";
import { Hash, Plus } from "lucide-react";

import { CreateChannelDialog } from "@/components/create-channel-dialog";
import {
	Sidebar,
	SidebarContent,
	SidebarFooter,
	SidebarGroup,
	SidebarGroupAction,
	SidebarGroupContent,
	SidebarGroupLabel,
	SidebarHeader,
	SidebarMenu,
	SidebarMenuBadge,
	SidebarMenuButton,
	SidebarMenuItem,
} from "@/components/ui/sidebar";
import { useAppSidebarViewModel } from "@/components/use-app-sidebar-view-model";
import { UserMenu } from "@/components/user-menu";

export function AppSidebar() {
	const vm = useAppSidebarViewModel();
	const [createOpen, setCreateOpen] = useState(false);

	return (
		<Sidebar variant="inset" collapsible="icon">
			<SidebarHeader>
				<div className="flex items-center gap-2 px-2 py-1">
					<span className="font-semibold">bcmd</span>
				</div>
			</SidebarHeader>
			<SidebarContent>
				<SidebarGroup>
					<SidebarGroupLabel>Channels</SidebarGroupLabel>
					<SidebarGroupAction
						onClick={() => setCreateOpen(true)}
						title="Create channel"
					>
						<Plus />
					</SidebarGroupAction>
					<SidebarGroupContent>
						<SidebarMenu>
							{vm.channelList.length === 0 ? (
								<p className="px-2 py-1 text-xs text-muted-foreground">
									No channels yet
								</p>
							) : (
								vm.channelList.map((ch) => (
									<SidebarMenuItem key={ch.id}>
										<SidebarMenuButton
											asChild
											isActive={vm.currentChannelId === ch.id}
										>
											<Link
												to={"/channels/$channelId" as never}
												params={{ channelId: ch.id } as never}
											>
												<Hash className="h-4 w-4" />
												<span>{ch.name}</span>
												<SidebarMenuBadge>{ch.memberCount}</SidebarMenuBadge>
											</Link>
										</SidebarMenuButton>
									</SidebarMenuItem>
								))
							)}
						</SidebarMenu>
					</SidebarGroupContent>
				</SidebarGroup>
				<SidebarGroup className="mt-auto">
					<SidebarGroupContent>
						<SidebarMenu>
							<SidebarMenuItem>
								<SidebarMenuButton asChild>
									<Link to={"/settings" as never}>Settings</Link>
								</SidebarMenuButton>
							</SidebarMenuItem>
							<SidebarMenuItem>
								<SidebarMenuButton asChild>
									<Link to={"/about" as never}>About</Link>
								</SidebarMenuButton>
							</SidebarMenuItem>
						</SidebarMenu>
					</SidebarGroupContent>
				</SidebarGroup>
			</SidebarContent>
			<SidebarFooter>
				<UserMenu />
			</SidebarFooter>
			<CreateChannelDialog open={createOpen} onOpenChange={setCreateOpen} />
		</Sidebar>
	);
}
