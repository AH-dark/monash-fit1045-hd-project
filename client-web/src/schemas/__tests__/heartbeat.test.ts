import { describe, expect, it } from "vitest";

import { HeartbeatVariablesSchema } from "../heartbeat";

describe("HeartbeatVariablesSchema", () => {
	it("parses a valid clientId", () => {
		expect(HeartbeatVariablesSchema.parse({ clientId: "abc" })).toEqual({
			clientId: "abc",
		});
	});

	it("rejects an empty clientId", () => {
		expect(HeartbeatVariablesSchema.safeParse({ clientId: "" })).toEqual(
			expect.objectContaining({ success: false }),
		);
	});

	it("rejects a missing clientId key", () => {
		expect(HeartbeatVariablesSchema.safeParse({})).toEqual(
			expect.objectContaining({ success: false }),
		);
	});
});
