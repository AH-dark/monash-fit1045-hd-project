import { createElement, type PropsWithChildren } from "react";

import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { useCreateChannelMutation } from "@/hooks/use-create-channel-mutation";

const mocks = vi.hoisted(() => ({
	createChannel: vi.fn().mockResolvedValue({
		channelId: "ch-1",
		channelName: "test",
	}),
}));

vi.mock("@/api/broadcast/operations", () => ({
	createChannel: mocks.createChannel,
}));

function wrapper({ children }: PropsWithChildren) {
	const queryClient = new QueryClient();
	return createElement(QueryClientProvider, { client: queryClient }, children);
}

describe("useCreateChannelMutation", () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it("calls createChannel with mutation variables", async () => {
		const { result } = renderHook(() => useCreateChannelMutation(), {
			wrapper,
		});

		await act(async () => {
			await result.current.mutateAsync({
				clientId: "client-1",
				channelName: "test-channel",
			});
		});

		expect(mocks.createChannel).toHaveBeenCalledWith(
			"client-1",
			"test-channel",
		);
	});
});
