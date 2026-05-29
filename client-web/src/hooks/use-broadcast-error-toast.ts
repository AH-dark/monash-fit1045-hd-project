import { useNavigate } from "@tanstack/react-router";
import { useEffect } from "react";
import { toast } from "sonner";

import type { BroadcastError } from "@/api/broadcast/errors";
import { getBroadcastErrorPresentation } from "@/lib/error-messages";

export function useBroadcastErrorToast(err: BroadcastError | null): void {
	const navigate = useNavigate();

	useEffect(() => {
		if (!err) return;
		const presentation = getBroadcastErrorPresentation(err);
		const action = presentation.action;
		toast.error(presentation.message, {
			action: action
				? {
						label: action.label,
						onClick: () => {
							if (
								action.description === "redirect-login" ||
								action.description === "reconnect"
							) {
								void navigate({ to: "/login" as never });
							} else if (action.description === "navigate-home") {
								void navigate({ to: "/" as never });
							}
						},
					}
				: undefined,
		});
	}, [err, navigate]);
}
