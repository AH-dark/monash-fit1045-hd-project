import { Enum } from "enum-plus";
import * as z from "zod";

export const AuthStatus = Enum({
	Idle: "idle",
	Connecting: "connecting",
	Connected: "connected",
	Disconnected: "disconnected",
});

export const AuthStatusSchema = z.enum([...AuthStatus.values] as [
	string,
	...string[],
]);

// AuthStatusValue avoids a TS identifier collision with the runtime constant AuthStatus above.
// Consumers needing the literal union type should use AuthStatusValue (or (typeof AuthStatus.values)[number]).
export type AuthStatusValue = (typeof AuthStatus.values)[number];

export const AuthStateSchema = z.object({
	status: AuthStatusSchema,
	clientId: z.string().nullable(),
	username: z.string().nullable(),
});

export type AuthState = z.infer<typeof AuthStateSchema>;

export const UsernameInputSchema = z.object({
	username: z
		.string()
		.min(1, "Username is required")
		.max(32)
		.regex(/^[a-zA-Z0-9_-]+$/, "Only letters, numbers, _ and - allowed"),
});

export type UsernameInput = z.infer<typeof UsernameInputSchema>;
