import { describe, expect, expectTypeOf, it } from "vitest";

import {
	type AuthState,
	AuthStateSchema,
	AuthStatus,
	AuthStatusSchema,
	type AuthStatusValue,
} from "../auth";

describe("AuthStatus enum-plus", () => {
	it("has the correct values array", () => {
		expect(AuthStatus.values).toEqual([
			"idle",
			"connecting",
			"connected",
			"disconnected",
		]);
	});

	it("exposes named keys mapping to string values", () => {
		expect(AuthStatus.Idle).toBe("idle");
		expect(AuthStatus.Connecting).toBe("connecting");
		expect(AuthStatus.Connected).toBe("connected");
		expect(AuthStatus.Disconnected).toBe("disconnected");
	});
});

describe("AuthStatusSchema", () => {
	it("parses a valid status", () => {
		expect(AuthStatusSchema.parse("connected")).toBe("connected");
	});

	it("rejects an unknown status", () => {
		expect(AuthStatusSchema.safeParse("bogus")).toEqual(
			expect.objectContaining({ success: false }),
		);
	});

	it("AuthStatusValue resolves to the correct literal union", () => {
		expectTypeOf<AuthStatusValue>().toEqualTypeOf<
			"idle" | "connecting" | "connected" | "disconnected"
		>();
	});
});

describe("AuthStateSchema", () => {
	it("parses a valid auth state", () => {
		const validState = {
			status: "connected",
			clientId: "client-123",
			username: "alice",
		};
		expect(AuthStateSchema.parse(validState)).toEqual(validState);
	});

	it("parses a state with null clientId and username", () => {
		const idleState = { status: "idle", clientId: null, username: null };
		expect(AuthStateSchema.parse(idleState)).toEqual(idleState);
	});

	it("AuthState type is structurally correct", () => {
		expectTypeOf<AuthState>().toEqualTypeOf<{
			status: string;
			clientId: string | null;
			username: string | null;
		}>();
	});
});
