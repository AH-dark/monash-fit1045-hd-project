import { useMemo } from "react";

import { useRouterState } from "@tanstack/react-router";
import { useShallow } from "zustand/react/shallow";

import type { BroadcastError } from "@/api/broadcast/errors";
import { useChannels } from "@/hooks/use-channels";
import type { Channel } from "@/schemas/channel";
import { useAuthStore } from "@/stores/auth-store";

export interface AppSidebarViewModel {
	readonly channelList: Channel[];
	readonly isLoading: boolean;
	readonly error: BroadcastError | null;
	readonly currentChannelId: string | null;
}

export function useAppSidebarViewModel(): AppSidebarViewModel {
	const clientId = useAuthStore(useShallow((s) => s.clientId));
	const { channels, error } = useChannels(clientId);
	const routerState = useRouterState();

	const channelList = useMemo(() => Array.from(channels.values()), [channels]);

	const currentChannelId = useMemo(() => {
		const match = routerState.location.pathname.match(/\/channels\/([^/]+)/);
		return match?.[1] ?? null;
	}, [routerState.location.pathname]);

	return useMemo(
		() => ({
			channelList,
			isLoading: false,
			error,
			currentChannelId,
		}),
		[channelList, error, currentChannelId],
	);
}
