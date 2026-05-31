import { useState } from "react";

import { Link } from "@tanstack/react-router";
import { ChevronUp } from "lucide-react";

import { DisconnectAlertDialog } from "@/components/disconnect-alert-dialog";
import { ThemeSwitcher } from "@/components/theme-switcher";
import { Avatar, AvatarFallback } from "@/components/ui/avatar";
import {
	DropdownMenu,
	DropdownMenuContent,
	DropdownMenuItem,
	DropdownMenuLabel,
	DropdownMenuSeparator,
	DropdownMenuSub,
	DropdownMenuSubContent,
	DropdownMenuSubTrigger,
	DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import { SidebarMenuButton } from "@/components/ui/sidebar";

import { useUserMenuViewModel } from "./use-user-menu-view-model";

export function UserMenu() {
	const vm = useUserMenuViewModel();
	const [disconnectOpen, setDisconnectOpen] = useState(false);

	return (
		<>
			<DropdownMenu>
				<DropdownMenuTrigger asChild>
					<SidebarMenuButton size="lg">
						<Avatar size="sm">
							<AvatarFallback>{vm.initial}</AvatarFallback>
						</Avatar>
						<span className="truncate">{vm.username ?? "Guest"}</span>
						<ChevronUp className="ml-auto" />
					</SidebarMenuButton>
				</DropdownMenuTrigger>
				<DropdownMenuContent
					side="top"
					align="start"
					className="w-(--radix-popper-anchor-width) min-w-56"
				>
					<DropdownMenuLabel className="truncate">
						{vm.username ?? "Guest"}
					</DropdownMenuLabel>
					<DropdownMenuItem asChild>
						<Link to={"/settings" as never}>Settings</Link>
					</DropdownMenuItem>
					<DropdownMenuItem asChild>
						<Link to={"/about" as never}>About</Link>
					</DropdownMenuItem>
					<DropdownMenuSeparator />
					<DropdownMenuSub>
						<DropdownMenuSubTrigger>Theme</DropdownMenuSubTrigger>
						<DropdownMenuSubContent>
							<ThemeSwitcher variant="menu" />
						</DropdownMenuSubContent>
					</DropdownMenuSub>
					<DropdownMenuSeparator />
					<DropdownMenuItem
						variant="destructive"
						onSelect={(event) => {
							event.preventDefault();
							setDisconnectOpen(true);
						}}
					>
						Disconnect
					</DropdownMenuItem>
				</DropdownMenuContent>
			</DropdownMenu>
			<DisconnectAlertDialog
				open={disconnectOpen}
				onOpenChange={setDisconnectOpen}
			/>
		</>
	);
}
