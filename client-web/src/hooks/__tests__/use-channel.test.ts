import { renderHook } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { useChannel } from "#/hooks/use-channel";
import { useMessagesStore } from "#/stores/messages-store";

const emptyMessages: [] = [];

vi.mock("#/api/broadcast/operations", () => ({
	joinChannel: vi.fn().mockResolvedValue(undefined),
	leaveChannel: vi.fn().mockResolvedValue(undefined),
	subscribeToChannel: vi.fn().mockReturnValue({
		[Symbol.asyncIterator]: () => ({ next: () => new Promise(() => {}) }),
	}),
}));

describe("useChannel", () => {
	beforeEach(() => {
		useMessagesStore.setState({ messages: new Map([["channel-1", emptyMessages]]) });
		vi.clearAllMocks();
	});

	it("returns messages and isConnected", () => {
		const { result } = renderHook(() => useChannel("client-1", "channel-1"));

		expect(result.current.messages).toEqual([]);
		expect(typeof result.current.isConnected).toBe("boolean");
	});

	it("isConnected is false initially", () => {
		const { result } = renderHook(() => useChannel(null, "channel-1"));

		expect(result.current.isConnected).toBe(false);
		expect(result.current.messages).toEqual([]);
	});
});
