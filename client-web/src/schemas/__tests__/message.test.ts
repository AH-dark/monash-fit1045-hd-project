import { describe, expect, expectTypeOf, it } from "vitest";

import {
	type Message,
	MessageSchema,
	SendMessageVariablesSchema,
} from "../message";

const validMessage = {
	messageId: "m1",
	channelId: "c1",
	senderId: "s1",
	senderName: "Alice",
	content: "hi",
	timestamp: 1700000000,
};

describe("MessageSchema", () => {
	it("parses a valid message object", () => {
		expect(MessageSchema.parse(validMessage)).toEqual(validMessage);
	});

	it("rejects a negative timestamp", () => {
		expect(MessageSchema.safeParse({ ...validMessage, timestamp: -1 })).toEqual(
			expect.objectContaining({ success: false }),
		);
	});

	it("rejects an empty messageId", () => {
		expect(MessageSchema.safeParse({ ...validMessage, messageId: "" })).toEqual(
			expect.objectContaining({ success: false }),
		);
	});

	it("Message type is structurally correct", () => {
		expectTypeOf<Message>().toEqualTypeOf<{
			messageId: string;
			channelId: string;
			senderId: string;
			senderName: string;
			content: string;
			timestamp: number;
		}>();
	});
});

describe("SendMessageVariablesSchema", () => {
	it("parses valid send-message variables", () => {
		const valid = { clientId: "a", channelId: "b", content: "hello" };
		expect(SendMessageVariablesSchema.parse(valid)).toEqual(valid);
	});

	it("rejects an empty content", () => {
		expect(
			SendMessageVariablesSchema.safeParse({
				clientId: "a",
				channelId: "b",
				content: "",
			}),
		).toEqual(expect.objectContaining({ success: false }));
	});
});
