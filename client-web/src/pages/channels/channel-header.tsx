import { useState } from "react";

import {
	AlertDialog,
	AlertDialogAction,
	AlertDialogCancel,
	AlertDialogContent,
	AlertDialogDescription,
	AlertDialogFooter,
	AlertDialogHeader,
	AlertDialogTitle,
} from "@/components/ui/alert-dialog";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { toastBroadcastPromise } from "@/lib/toast-helpers";

import type { ChannelViewModel } from "./use-channel-view-model";

interface ChannelHeaderProps {
	vm: ChannelViewModel;
}

export function ChannelHeader({ vm }: ChannelHeaderProps) {
	const [confirmOpen, setConfirmOpen] = useState(false);

	const handleConfirmLeave = () => {
		setConfirmOpen(false);
		void toastBroadcastPromise(vm.leaveChannel(), {
			loading: "Leaving channel...",
			success: "Left channel",
		});
	};

	return (
		<header className="flex shrink-0 items-center justify-between gap-3 border-b px-4 py-3">
			<div className="flex min-w-0 items-center gap-2">
				<h2 className="truncate text-base font-semibold">
					{vm.channelName ?? "Channel"}
				</h2>
				{vm.memberCount !== undefined && (
					<Badge variant="secondary">
						{vm.memberCount} {vm.memberCount === 1 ? "member" : "members"}
					</Badge>
				)}
			</div>
			<Button
				variant="outline"
				size="sm"
				onClick={() => setConfirmOpen(true)}
				disabled={!vm.isConnected}
			>
				Leave
			</Button>
			<AlertDialog open={confirmOpen} onOpenChange={setConfirmOpen}>
				<AlertDialogContent>
					<AlertDialogHeader>
						<AlertDialogTitle>Leave this channel?</AlertDialogTitle>
						<AlertDialogDescription>
							You will stop receiving messages from{" "}
							<span className="font-medium">
								{vm.channelName ?? "this channel"}
							</span>
							. You can rejoin later.
						</AlertDialogDescription>
					</AlertDialogHeader>
					<AlertDialogFooter>
						<AlertDialogCancel>Cancel</AlertDialogCancel>
						<AlertDialogAction
							variant="destructive"
							onClick={handleConfirmLeave}
						>
							Leave
						</AlertDialogAction>
					</AlertDialogFooter>
				</AlertDialogContent>
			</AlertDialog>
		</header>
	);
}
