import { beforeEach, describe, expect, it } from "vitest";

import { useMessagesStore } from "@/stores/messages-store";

function sampleMessage(id: string, channelId = "c1") {
	return {
		messageId: id,
		channelId,
		senderId: "u1",
		senderName: "alice",
		content: `hello ${id}`,
		timestamp: 1700000000000,
	};
}

describe("messages-store", () => {
	beforeEach(() => {
		useMessagesStore.getState().reset();
	});

	it("addMessage appends to channel's messages", () => {
		useMessagesStore.getState().addMessage(sampleMessage("m1"));
		useMessagesStore.getState().addMessage(sampleMessage("m2"));
		const channelMsgs = useMessagesStore.getState().messages.get("c1");
		expect(channelMsgs).toHaveLength(2);
		expect(channelMsgs?.[0]?.messageId).toBe("m1");
		expect(channelMsgs?.[1]?.messageId).toBe("m2");
	});

	it("setMessages replaces channel messages", () => {
		useMessagesStore.getState().addMessage(sampleMessage("m1"));
		useMessagesStore
			.getState()
			.setMessages("c1", [sampleMessage("m2"), sampleMessage("m3")]);
		const channelMsgs = useMessagesStore.getState().messages.get("c1");
		expect(channelMsgs).toHaveLength(2);
		expect(channelMsgs?.[0]?.messageId).toBe("m2");
		expect(channelMsgs?.[1]?.messageId).toBe("m3");
	});

	it("clearMessages removes channel messages", () => {
		useMessagesStore.getState().addMessage(sampleMessage("m1"));
		useMessagesStore.getState().addMessage(sampleMessage("m2", "c2"));
		useMessagesStore.getState().clearMessages("c1");
		const state = useMessagesStore.getState();
		expect(state.messages.has("c1")).toBe(false);
		expect(state.messages.has("c2")).toBe(true);
	});

	it("reset clears all messages", () => {
		useMessagesStore.getState().addMessage(sampleMessage("m1"));
		useMessagesStore.getState().addMessage(sampleMessage("m2", "c2"));
		useMessagesStore.getState().reset();
		expect(useMessagesStore.getState().messages.size).toBe(0);
	});
});
