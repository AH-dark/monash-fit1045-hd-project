import { describe, expect, expectTypeOf, it } from "vitest";

import {
	BroadcastErrorKind,
	BroadcastErrorKindSchema,
} from "../broadcast-error";

describe("BroadcastErrorKind enum-plus", () => {
	it("has the correct values array", () => {
		expect(BroadcastErrorKind.values).toEqual([
			"client-not-found",
			"channel-not-found",
			"invalid-argument",
			"unavailable",
			"unknown",
		]);
	});

	it("exposes named keys mapping to string values", () => {
		expect(BroadcastErrorKind.ClientNotFound).toBe("client-not-found");
		expect(BroadcastErrorKind.ChannelNotFound).toBe("channel-not-found");
		expect(BroadcastErrorKind.InvalidArgument).toBe("invalid-argument");
		expect(BroadcastErrorKind.Unavailable).toBe("unavailable");
		expect(BroadcastErrorKind.Unknown).toBe("unknown");
	});
});

describe("BroadcastErrorKindSchema", () => {
	it("parses a valid error kind", () => {
		expect(BroadcastErrorKindSchema.parse("client-not-found")).toBe(
			"client-not-found",
		);
	});

	it("rejects an unknown error kind", () => {
		expect(BroadcastErrorKindSchema.safeParse("bogus")).toEqual(
			expect.objectContaining({ success: false }),
		);
	});

	it("BroadcastErrorKind type resolves to the correct 5-member literal union", () => {
		expectTypeOf<BroadcastErrorKind>().toEqualTypeOf<
			| "client-not-found"
			| "channel-not-found"
			| "invalid-argument"
			| "unavailable"
			| "unknown"
		>();
	});
});
