import { createElement, type PropsWithChildren } from "react";

import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { useLeaveChannelMutation } from "@/hooks/use-leave-channel-mutation";

const mocks = vi.hoisted(() => ({
	leaveChannel: vi.fn().mockResolvedValue(undefined),
}));

vi.mock("@/api/broadcast/operations", () => ({
	leaveChannel: mocks.leaveChannel,
}));

function wrapper({ children }: PropsWithChildren) {
	const queryClient = new QueryClient();
	return createElement(QueryClientProvider, { client: queryClient }, children);
}

describe("useLeaveChannelMutation", () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it("calls leaveChannel with mutation variables", async () => {
		const { result } = renderHook(() => useLeaveChannelMutation(), { wrapper });

		await act(async () => {
			await result.current.mutateAsync({
				clientId: "client-1",
				channelId: "channel-1",
			});
		});

		expect(mocks.leaveChannel).toHaveBeenCalledWith("client-1", "channel-1");
	});
});
