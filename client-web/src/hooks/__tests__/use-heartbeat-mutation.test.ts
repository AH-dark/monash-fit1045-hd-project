import { createElement, type PropsWithChildren } from "react";

import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { act, renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { useHeartbeatMutation } from "@/hooks/use-heartbeat-mutation";

const mocks = vi.hoisted(() => ({
	heartbeat: vi.fn().mockResolvedValue(undefined),
}));

vi.mock("@/api/broadcast/operations", () => ({
	heartbeat: mocks.heartbeat,
}));

function wrapper({ children }: PropsWithChildren) {
	const queryClient = new QueryClient();
	return createElement(QueryClientProvider, { client: queryClient }, children);
}

describe("useHeartbeatMutation", () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it("calls heartbeat with mutation variables", async () => {
		const { result } = renderHook(() => useHeartbeatMutation(), { wrapper });

		await act(async () => {
			await result.current.mutateAsync("client-1");
		});

		expect(mocks.heartbeat).toHaveBeenCalledWith("client-1");
	});
});
