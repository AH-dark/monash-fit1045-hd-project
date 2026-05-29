import * as z from "zod";

export const MessageSchema = z.object({
	messageId: z.string().min(1),
	channelId: z.string().min(1),
	senderId: z.string().min(1),
	senderName: z.string(),
	content: z.string(),
	timestamp: z.number().int().nonnegative(),
});

export type Message = z.infer<typeof MessageSchema>;

export const SendMessageVariablesSchema = z.object({
	clientId: z.string().min(1),
	channelId: z.string().min(1),
	content: z.string().min(1),
});

export type SendMessageVariables = z.infer<typeof SendMessageVariablesSchema>;
