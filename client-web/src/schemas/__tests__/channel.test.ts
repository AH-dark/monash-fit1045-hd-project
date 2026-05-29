import { describe, expect, expectTypeOf, it } from "vitest";

import {
	type Channel,
	ChannelSchema,
	CreateChannelVariablesSchema,
	LeaveChannelVariablesSchema,
} from "../channel";

describe("ChannelSchema", () => {
	it("parses a valid channel object", () => {
		const valid = { id: "c1", name: "general", memberCount: 3 };
		expect(ChannelSchema.parse(valid)).toEqual(valid);
	});

	it("rejects an empty id", () => {
		expect(
			ChannelSchema.safeParse({ id: "", name: "x", memberCount: 0 }),
		).toEqual(expect.objectContaining({ success: false }));
	});

	it("rejects a negative memberCount", () => {
		expect(
			ChannelSchema.safeParse({ id: "c1", name: "x", memberCount: -1 }),
		).toEqual(expect.objectContaining({ success: false }));
	});

	it("Channel type is structurally compatible", () => {
		expectTypeOf<Channel>().toEqualTypeOf<{
			id: string;
			name: string;
			memberCount: number;
		}>();
	});
});

describe("CreateChannelVariablesSchema", () => {
	it("parses valid create-channel variables", () => {
		const valid = { clientId: "a", channelName: "b" };
		expect(CreateChannelVariablesSchema.parse(valid)).toEqual(valid);
	});

	it("rejects an empty channelName", () => {
		expect(
			CreateChannelVariablesSchema.safeParse({
				clientId: "a",
				channelName: "",
			}),
		).toEqual(expect.objectContaining({ success: false }));
	});
});

describe("LeaveChannelVariablesSchema", () => {
	it("parses valid leave-channel variables", () => {
		const valid = { clientId: "a", channelId: "b" };
		expect(LeaveChannelVariablesSchema.parse(valid)).toEqual(valid);
	});

	it("rejects an empty clientId", () => {
		expect(
			LeaveChannelVariablesSchema.safeParse({ clientId: "", channelId: "b" }),
		).toEqual(expect.objectContaining({ success: false }));
	});
});
