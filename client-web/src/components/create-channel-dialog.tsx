import { type FormEvent, useState } from "react";

import { useNavigate } from "@tanstack/react-router";

import { Button } from "@/components/ui/button";
import {
	Dialog,
	DialogContent,
	DialogDescription,
	DialogFooter,
	DialogHeader,
	DialogTitle,
} from "@/components/ui/dialog";
import {
	Field,
	FieldDescription,
	FieldError,
	FieldGroup,
	FieldLabel,
} from "@/components/ui/field";
import { Input } from "@/components/ui/input";
import { toastBroadcastPromise } from "@/lib/toast-helpers";

import { useCreateChannelDialogViewModel } from "./use-create-channel-dialog-view-model";

export interface CreateChannelDialogProps {
	open: boolean;
	onOpenChange: (open: boolean) => void;
}

export function CreateChannelDialog({
	open,
	onOpenChange,
}: CreateChannelDialogProps) {
	const navigate = useNavigate();
	const vm = useCreateChannelDialogViewModel();
	const [channelName, setChannelName] = useState("");
	const [errorMessage, setErrorMessage] = useState<string | null>(null);

	const resetInput = (): void => {
		setChannelName("");
		setErrorMessage(null);
	};

	const handleOpenChange = (nextOpen: boolean): void => {
		if (!nextOpen) {
			resetInput();
		}
		onOpenChange(nextOpen);
	};

	const handleSubmit = async (
		event: FormEvent<HTMLFormElement>,
	): Promise<void> => {
		event.preventDefault();
		const parsed = vm.schema.safeParse({ channelName });
		if (!parsed.success) {
			const message =
				parsed.error.issues[0]?.message ?? "Invalid channel name";
			setErrorMessage(message);
			return;
		}

		setErrorMessage(null);
		const result = await toastBroadcastPromise(
			vm.createChannel(parsed.data.channelName),
			{ loading: "Creating...", success: "Channel created!" },
		).catch(() => null);

		if (!result) {
			return;
		}

		resetInput();
		onOpenChange(false);
		await navigate({ to: `/channels/${result.channelId}` as never });
	};

	const invalid = errorMessage !== null;

	return (
		<Dialog open={open} onOpenChange={handleOpenChange}>
			<DialogContent>
				<form onSubmit={handleSubmit}>
					<DialogHeader>
						<DialogTitle>Create channel</DialogTitle>
						<DialogDescription>
							Choose a unique name. Others can join after it's created.
						</DialogDescription>
					</DialogHeader>
					<FieldGroup className="py-4">
						<Field data-invalid={invalid ? true : undefined}>
							<FieldLabel htmlFor="create-channel-name">
								Channel name
							</FieldLabel>
							<Input
								id="create-channel-name"
								value={channelName}
								onChange={(event) => {
									setChannelName(event.target.value);
									if (errorMessage) {
										setErrorMessage(null);
									}
								}}
								placeholder="e.g. general"
								autoComplete="off"
								disabled={vm.isCreating}
								aria-invalid={invalid || undefined}
								maxLength={50}
							/>
							<FieldDescription>
								Letters, numbers, spaces, _ and - allowed (1-50 characters).
							</FieldDescription>
							{errorMessage ? <FieldError>{errorMessage}</FieldError> : null}
						</Field>
					</FieldGroup>
					<DialogFooter>
						<Button
							type="button"
							variant="outline"
							onClick={() => handleOpenChange(false)}
							disabled={vm.isCreating}
						>
							Cancel
						</Button>
						<Button type="submit" disabled={vm.isCreating}>
							Create
						</Button>
					</DialogFooter>
				</form>
			</DialogContent>
		</Dialog>
	);
}
