import { useState } from "react";

import { Copy } from "lucide-react";

import { DisconnectAlertDialog } from "@/components/disconnect-alert-dialog";
import { ThemeSwitcher } from "@/components/theme-switcher";
import { Button } from "@/components/ui/button";
import {
	Card,
	CardContent,
	CardDescription,
	CardHeader,
	CardTitle,
} from "@/components/ui/card";

import { useSettingsViewModel } from "./use-settings-view-model";

export function SettingsPage() {
	const vm = useSettingsViewModel();
	const [disconnectOpen, setDisconnectOpen] = useState(false);

	const handleCopyClientId = (): void => {
		if (vm.clientId) {
			void navigator.clipboard.writeText(vm.clientId);
		}
	};

	return (
		<div className="container mx-auto max-w-2xl space-y-6 p-6">
			<div>
				<h1 className="text-2xl font-semibold tracking-tight">Settings</h1>
				<p className="text-sm text-muted-foreground">
					Manage your preferences and session.
				</p>
			</div>

			<Card>
				<CardHeader>
					<CardTitle>Appearance</CardTitle>
					<CardDescription>Choose how the interface looks.</CardDescription>
				</CardHeader>
				<CardContent>
					<ThemeSwitcher variant="standalone" />
				</CardContent>
			</Card>

			<Card>
				<CardHeader>
					<CardTitle>Session</CardTitle>
					<CardDescription>
						Details about your current connection.
					</CardDescription>
				</CardHeader>
				<CardContent className="space-y-4">
					<div className="flex flex-col gap-1">
						<span className="text-sm font-medium">Username</span>
						<span className="text-sm text-muted-foreground">
							{vm.username ?? "—"}
						</span>
					</div>
					<div className="flex flex-col gap-1">
						<span className="text-sm font-medium">Client ID</span>
						<div className="flex items-center gap-2">
							<code className="flex-1 truncate rounded-md bg-muted px-2 py-1 text-xs">
								{vm.clientId ?? "—"}
							</code>
							<Button
								variant="outline"
								size="icon-sm"
								onClick={handleCopyClientId}
								disabled={!vm.clientId}
								aria-label="Copy client ID"
							>
								<Copy />
							</Button>
						</div>
					</div>
				</CardContent>
			</Card>

			<Card>
				<CardHeader>
					<CardTitle>Connection</CardTitle>
					<CardDescription>Network and session controls.</CardDescription>
				</CardHeader>
				<CardContent className="space-y-4">
					<div className="flex flex-col gap-1">
						<span className="text-sm font-medium">Heartbeat interval</span>
						<span className="text-sm text-muted-foreground">
							{vm.heartbeatIntervalMs} ms
						</span>
					</div>
					<Button
						variant="destructive"
						onClick={() => setDisconnectOpen(true)}
						disabled={!vm.clientId}
					>
						Disconnect
					</Button>
				</CardContent>
			</Card>

			<DisconnectAlertDialog
				open={disconnectOpen}
				onOpenChange={setDisconnectOpen}
			/>
		</div>
	);
}
