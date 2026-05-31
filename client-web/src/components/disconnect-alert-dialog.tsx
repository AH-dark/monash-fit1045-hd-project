import { useNavigate } from "@tanstack/react-router";

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
import { toastBroadcastPromise } from "@/lib/toast-helpers";

import { useDisconnectViewModel } from "./use-disconnect-view-model";

export interface DisconnectAlertDialogProps {
	open: boolean;
	onOpenChange: (open: boolean) => void;
}

export function DisconnectAlertDialog({
	open,
	onOpenChange,
}: DisconnectAlertDialogProps) {
	const navigate = useNavigate();
	const vm = useDisconnectViewModel();

	const handleConfirm = async (): Promise<void> => {
		const succeeded = await toastBroadcastPromise(vm.disconnect(), {
			loading: "Disconnecting...",
			success: "Disconnected",
		}).then(
			() => true,
			() => false,
		);
		if (succeeded) {
			await navigate({ to: "/login" as never });
		}
	};

	return (
		<AlertDialog open={open} onOpenChange={onOpenChange}>
			<AlertDialogContent>
				<AlertDialogHeader>
					<AlertDialogTitle>Disconnect from bcmd?</AlertDialogTitle>
					<AlertDialogDescription>
						Your session will be cleared and you'll need to reconnect.
					</AlertDialogDescription>
				</AlertDialogHeader>
				<AlertDialogFooter>
					<AlertDialogCancel disabled={vm.isDisconnecting}>
						Cancel
					</AlertDialogCancel>
					<AlertDialogAction
						variant="destructive"
						disabled={vm.isDisconnecting}
						onClick={() => {
							void handleConfirm();
						}}
					>
						Disconnect
					</AlertDialogAction>
				</AlertDialogFooter>
			</AlertDialogContent>
		</AlertDialog>
	);
}
