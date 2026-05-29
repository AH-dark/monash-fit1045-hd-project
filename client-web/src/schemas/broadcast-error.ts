import { Enum } from "enum-plus";
import * as z from "zod";

export const BroadcastErrorKind = Enum({
	ClientNotFound: "client-not-found",
	ChannelNotFound: "channel-not-found",
	InvalidArgument: "invalid-argument",
	Unavailable: "unavailable",
	Unknown: "unknown",
});

export const BroadcastErrorKindSchema = z.enum([
	...BroadcastErrorKind.values,
] as [string, ...string[]]);

export type BroadcastErrorKind = (typeof BroadcastErrorKind.values)[number];
