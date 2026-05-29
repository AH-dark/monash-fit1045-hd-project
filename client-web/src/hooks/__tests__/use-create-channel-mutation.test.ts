import { createElement, type PropsWithChildren } from "react";

import { Code, ConnectError } from "@connectrpc/connect";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, expectTypeOf, it, vi } from "vitest";

import { BroadcastError } from "@/api/broadcast/errors";
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
	const queryClient = new QueryClient({
		defaultOptions: { mutations: { retry: false } },
	});
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

	it("returns channelId and channelName on success", async () => {
		mocks.createChannel.mockResolvedValueOnce({
			channelId: "ch-42",
			channelName: "general",
		});

		const { result } = renderHook(() => useCreateChannelMutation(), {
			wrapper,
		});

		let data: { channelId: string; channelName: string } | undefined;
		await act(async () => {
			data = await result.current.mutateAsync({
				clientId: "c",
				channelName: "general",
			});
		});

		expect(data).toEqual({ channelId: "ch-42", channelName: "general" });
		// Compile-time type check: data must have channelId and channelName as strings
		expectTypeOf(data).toEqualTypeOf<
			{ channelId: string; channelName: string } | undefined
		>();
	});

	it("maps ConnectError(NotFound, client message) to BroadcastError with kind client-not-found", async () => {
		const connectErr = new ConnectError("client not found", Code.NotFound);
		mocks.createChannel.mockRejectedValueOnce(connectErr);

		const { result } = renderHook(() => useCreateChannelMutation(), {
			wrapper,
		});

		let thrown: unknown;
		await act(async () => {
			try {
				await result.current.mutateAsync({
					clientId: "c",
					channelName: "general",
				});
			} catch (err) {
				thrown = err;
			}
		});

		expect(thrown).toBeInstanceOf(BroadcastError);
		expect((thrown as BroadcastError).kind).toBe("client-not-found");
	});
});
