import type { BroadcastError } from "@/api/broadcast/errors";

export type ActionDescription =
	| "redirect-login"
	| "reconnect"
	| "navigate-home"
	| "none";

export type ErrorPresentation = {
	type: "error" | "warning";
	message: string;
	action?: { label: string; description: ActionDescription };
};

export function getBroadcastErrorPresentation(
	err: BroadcastError,
): ErrorPresentation {
	const kind = err.kind;

	switch (kind) {
		case "client-not-found":
			return {
				type: "error",
				message: "Session expired. Please reconnect.",
				action: { label: "Reconnect", description: "redirect-login" },
			};
		case "channel-not-found":
			return {
				type: "error",
				message: "Channel no longer exists.",
				action: { label: "Go home", description: "navigate-home" },
			};
		case "invalid-argument":
			return {
				type: "error",
				message: `Invalid input: ${err.original.message}`,
			};
		case "unavailable":
			return {
				type: "error",
				message: "Server unavailable. Please try again.",
				action: { label: "Reconnect", description: "reconnect" },
			};
		case "unknown":
			return {
				type: "error",
				message: "Something went wrong.",
			};
		default: {
			const _: never = kind;
			return _;
		}
	}
}
