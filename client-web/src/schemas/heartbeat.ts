import * as z from "zod";

export const HeartbeatVariablesSchema = z.object({
	clientId: z.string().min(1),
});

export type HeartbeatVariables = z.infer<typeof HeartbeatVariablesSchema>;
