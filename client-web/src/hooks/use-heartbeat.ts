import { useEffect, useRef } from "react";

import { heartbeat as rpcHeartbeat } from "@/api/broadcast/operations";

export function useHeartbeat(clientId: string | null, intervalMs = 3000) {
	const clientIdRef = useRef(clientId);
	clientIdRef.current = clientId;

	useEffect(() => {
		if (!clientId) return;

		const timer = setInterval(async () => {
			const id = clientIdRef.current;
			if (!id) return;
			try {
				await rpcHeartbeat(id);
			} catch {
				// Heartbeat failures are non-fatal; the server will expire the client eventually.
			}
		}, intervalMs);

		return () => clearInterval(timer);
	}, [clientId, intervalMs]);
}
