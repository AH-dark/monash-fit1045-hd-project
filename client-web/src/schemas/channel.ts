import * as z from "zod";

export const ChannelSchema = z.object({
	id: z.string().min(1),
	name: z.string().min(1),
	memberCount: z.number().int().min(0),
});

export type Channel = z.infer<typeof ChannelSchema>;

export const CreateChannelVariablesSchema = z.object({
	clientId: z.string().min(1),
	channelName: z.string().min(1),
});

export type CreateChannelVariables = z.infer<
	typeof CreateChannelVariablesSchema
>;

export const LeaveChannelVariablesSchema = z.object({
	clientId: z.string().min(1),
	channelId: z.string().min(1),
});

export type LeaveChannelVariables = z.infer<typeof LeaveChannelVariablesSchema>;
