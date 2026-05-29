import { useEffect, useRef } from "react";

import { useNavigate } from "@tanstack/react-router";
import { toast } from "sonner";
import { useShallow } from "zustand/react/shallow";

import { useAuthStore } from "@/stores/auth-store";

const CONNECTION_LOST_TOAST_ID = "connection-lost";

export function ConnectionStatusWatcher(): null {
	const navigate = useNavigate();
	const status = useAuthStore(useShallow((s) => s.status));
	const prevStatusRef = useRef<string>(status);

	useEffect(() => {
		const prev = prevStatusRef.current;
		prevStatusRef.current = status;

		if (prev === "connected" && status === "disconnected") {
			toast.error("Connection lost. Reconnect?", {
				id: CONNECTION_LOST_TOAST_ID,
				duration: Number.POSITIVE_INFINITY,
				action: {
					label: "Reconnect",
					onClick: () => {
						void navigate({ to: "/login" as never });
					},
				},
			});
		} else if (status === "connected") {
			toast.dismiss(CONNECTION_LOST_TOAST_ID);
		}
	}, [status, navigate]);

	return null;
}
