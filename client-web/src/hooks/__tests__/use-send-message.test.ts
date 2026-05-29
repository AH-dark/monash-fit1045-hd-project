import { createElement, type PropsWithChildren } from "react";

import { Code, ConnectError } from "@connectrpc/connect";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, expectTypeOf, it, vi } from "vitest";

import { BroadcastError } from "@/api/broadcast/errors";
import { useSendMessageMutation } from "@/hooks/use-send-message";

const mocks = vi.hoisted(() => ({
	sendMessage: vi.fn().mockResolvedValue({ messageId: "msg-1" }),
}));

vi.mock("@/api/broadcast/operations", () => ({
	sendMessage: mocks.sendMessage,
}));

function wrapper({ children }: PropsWithChildren) {
	const queryClient = new QueryClient({
		defaultOptions: { mutations: { retry: false } },
	});
	return createElement(QueryClientProvider, { client: queryClient }, children);
}

describe("useSendMessageMutation", () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it("calls sendMessage with mutation variables", async () => {
		const { result } = renderHook(() => useSendMessageMutation(), { wrapper });

		await act(async () => {
			await result.current.mutateAsync({
				clientId: "client-1",
				channelId: "channel-1",
				content: "hello",
			});
		});

		expect(mocks.sendMessage).toHaveBeenCalledWith(
			"client-1",
			"channel-1",
			"hello",
		);
	});

	it("returns messageId on success", async () => {
		mocks.sendMessage.mockResolvedValueOnce({ messageId: "msg-42" });

		const { result } = renderHook(() => useSendMessageMutation(), { wrapper });

		let data: { messageId: string } | undefined;
		await act(async () => {
			data = await result.current.mutateAsync({
				clientId: "client-1",
				channelId: "channel-1",
				content: "hello",
			});
		});

		expect(data).toEqual({ messageId: "msg-42" });
		expectTypeOf(data).toEqualTypeOf<{ messageId: string } | undefined>();
	});

	it("maps ConnectError(Unavailable) to BroadcastError with kind unavailable", async () => {
		const connectErr = new ConnectError("fail", Code.Unavailable);
		mocks.sendMessage.mockRejectedValueOnce(connectErr);

		const { result } = renderHook(() => useSendMessageMutation(), { wrapper });

		let thrown: unknown;
		await act(async () => {
			try {
				await result.current.mutateAsync({
					clientId: "client-1",
					channelId: "channel-1",
					content: "hello",
				});
			} catch (err) {
				thrown = err;
			}
		});

		expect(thrown).toBeInstanceOf(BroadcastError);
		expect((thrown as BroadcastError).kind).toBe("unavailable");
	});
});
