import { toast } from "sonner";

import type { BroadcastError } from "@/api/broadcast/errors";
import { getBroadcastErrorPresentation } from "@/lib/error-messages";

function isBroadcastError(err: unknown): err is BroadcastError {
	return typeof err === "object" && err !== null && "kind" in err;
}

export function toastBroadcastPromise<T>(
	promise: Promise<T>,
	opts: { loading: string; success: string | ((data: T) => string) },
): Promise<T> {
	toast.promise(promise, {
		loading: opts.loading,
		success: opts.success,
		error: (err: unknown) => {
			if (isBroadcastError(err)) {
				const presentation = getBroadcastErrorPresentation(err);
				return presentation.message;
			}
			return "Something went wrong.";
		},
	});

	return promise;
}
