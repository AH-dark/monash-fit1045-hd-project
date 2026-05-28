import { renderHook } from "@testing-library/react";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { useHeartbeat } from "#/hooks/use-heartbeat";

const mocks = vi.hoisted(() => ({
	heartbeat: vi.fn().mockResolvedValue(undefined),
}));

vi.mock("#/api/broadcast/operations", () => ({
	heartbeat: mocks.heartbeat,
}));

describe("useHeartbeat", () => {
	beforeEach(() => {
		vi.useFakeTimers();
		vi.clearAllMocks();
	});

	afterEach(() => {
		vi.useRealTimers();
	});

	it("does not start interval when clientId is null", () => {
		renderHook(() => useHeartbeat(null));

		vi.advanceTimersByTime(3000);

		expect(mocks.heartbeat).not.toHaveBeenCalled();
	});

	it("starts interval when clientId is provided", async () => {
		renderHook(() => useHeartbeat("client-1", 1000));

		await vi.advanceTimersByTimeAsync(1000);

		expect(mocks.heartbeat).toHaveBeenCalledWith("client-1");
	});
});
