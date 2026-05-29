import { Code, ConnectError } from "@connectrpc/connect";

import type { BroadcastErrorKind } from "@/schemas/broadcast-error";

export class BroadcastError extends Error {
	constructor(
		public kind: BroadcastErrorKind,
		public code: Code,
		public original: ConnectError,
	) {
		super(original.message);
		this.name = "BroadcastError";
	}
}

export function mapError(err: unknown): BroadcastError {
	if (err instanceof ConnectError) {
		if (err.code === Code.NotFound && /client/i.test(err.message)) {
			return new BroadcastError("client-not-found", Code.NotFound, err);
		}
		if (err.code === Code.NotFound && /channel/i.test(err.message)) {
			return new BroadcastError("channel-not-found", Code.NotFound, err);
		}
		if (err.code === Code.InvalidArgument) {
			return new BroadcastError("invalid-argument", Code.InvalidArgument, err);
		}
		if (err.code === Code.Unavailable) {
			return new BroadcastError("unavailable", Code.Unavailable, err);
		}
		return new BroadcastError("unknown", err.code, err);
	}
	const original = new ConnectError(String(err), Code.Unknown);
	return new BroadcastError("unknown", Code.Unknown, original);
}

export function isClientNotFound(err: unknown): boolean {
	return err instanceof BroadcastError && err.kind === "client-not-found";
}
