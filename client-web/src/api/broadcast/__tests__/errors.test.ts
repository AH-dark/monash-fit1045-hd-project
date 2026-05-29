import { Code, ConnectError } from "@connectrpc/connect";
import { describe, expect, it } from "vitest";

import {
	BroadcastError,
	isClientNotFound,
	mapError,
} from "@/api/broadcast/errors";

describe("broadcast/errors", () => {
	describe("mapError", () => {
		it("maps Code.NotFound with 'client' to client-not-found", () => {
			const err = new ConnectError("client not found", Code.NotFound);
			const result = mapError(err);
			expect(result).toBeInstanceOf(BroadcastError);
			expect(result.kind).toBe("client-not-found");
			expect(result.code).toBe(Code.NotFound);
			expect(result.original).toBe(err);
		});

		it("maps Code.NotFound with 'channel' to channel-not-found", () => {
			const err = new ConnectError("channel does not exist", Code.NotFound);
			const result = mapError(err);
			expect(result.kind).toBe("channel-not-found");
			expect(result.code).toBe(Code.NotFound);
		});

		it("maps Code.InvalidArgument to invalid-argument", () => {
			const err = new ConnectError("bad request", Code.InvalidArgument);
			const result = mapError(err);
			expect(result.kind).toBe("invalid-argument");
			expect(result.code).toBe(Code.InvalidArgument);
		});

		it("maps Code.Unavailable to unavailable", () => {
			const err = new ConnectError("service down", Code.Unavailable);
			const result = mapError(err);
			expect(result.kind).toBe("unavailable");
			expect(result.code).toBe(Code.Unavailable);
		});

		it("maps unrecognized ConnectError code to unknown", () => {
			const err = new ConnectError("oops", Code.Internal);
			const result = mapError(err);
			expect(result.kind).toBe("unknown");
			expect(result.code).toBe(Code.Internal);
		});

		it("maps non-ConnectError to unknown", () => {
			const result = mapError(new Error("plain error"));
			expect(result.kind).toBe("unknown");
			expect(result.code).toBe(Code.Unknown);
			expect(result.original).toBeInstanceOf(ConnectError);
		});

		it("wraps non-Error throwables into a ConnectError", () => {
			const result = mapError("string thrown");
			expect(result.kind).toBe("unknown");
			expect(result.code).toBe(Code.Unknown);
			expect(result.original).toBeInstanceOf(ConnectError);
		});
	});

	describe("isClientNotFound", () => {
		it("returns true for client-not-found BroadcastError", () => {
			const err = mapError(new ConnectError("client missing", Code.NotFound));
			expect(isClientNotFound(err)).toBe(true);
		});

		it("returns false for other BroadcastError kinds", () => {
			const err = mapError(new ConnectError("bad", Code.InvalidArgument));
			expect(isClientNotFound(err)).toBe(false);
		});

		it("returns false for non-BroadcastError values", () => {
			expect(isClientNotFound(new Error("plain"))).toBe(false);
			expect(isClientNotFound(null)).toBe(false);
			expect(isClientNotFound(undefined)).toBe(false);
			expect(isClientNotFound("client-not-found")).toBe(false);
		});
	});
});
