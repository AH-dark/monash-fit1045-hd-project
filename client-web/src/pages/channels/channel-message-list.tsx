import { useCallback, useRef } from "react";

import { Avatar, AvatarFallback } from "@/components/ui/avatar";
import { Empty } from "@/components/ui/empty";
import { ScrollArea } from "@/components/ui/scroll-area";
import { useAutoScrollBottom } from "@/hooks/use-auto-scroll-bottom";
import type { Message } from "@/schemas/message";

import type { ChannelViewModel } from "./use-channel-view-model";

interface ChannelMessageListProps {
	vm: ChannelViewModel;
}

function formatTimestamp(ms: number): string {
	const date = new Date(ms);
	const hours = date.getHours().toString().padStart(2, "0");
	const minutes = date.getMinutes().toString().padStart(2, "0");
	return `${hours}:${minutes}`;
}

function getInitial(name: string): string {
	const trimmed = name.trim();
	if (trimmed.length === 0) return "?";
	return trimmed.charAt(0).toUpperCase();
}

interface MessageRowProps {
	message: Message;
}

function MessageRow({ message }: MessageRowProps) {
	return (
		<div className="flex items-start gap-3 px-4 py-2">
			<Avatar size="sm">
				<AvatarFallback>{getInitial(message.senderName)}</AvatarFallback>
			</Avatar>
			<div className="flex min-w-0 flex-1 flex-col gap-0.5">
				<div className="flex items-baseline gap-2">
					<span className="truncate text-sm font-medium">
						{message.senderName}
					</span>
					<span className="text-xs text-muted-foreground">
						{formatTimestamp(message.timestamp)}
					</span>
				</div>
				<p className="text-sm whitespace-pre-wrap break-words">
					{message.content}
				</p>
			</div>
		</div>
	);
}

export function ChannelMessageList({ vm }: ChannelMessageListProps) {
	const viewportRef = useRef<HTMLDivElement | null>(null);

	const setScrollRoot = useCallback((node: HTMLDivElement | null) => {
		if (node) {
			viewportRef.current = node.querySelector<HTMLDivElement>(
				'[data-slot="scroll-area-viewport"]',
			);
		} else {
			viewportRef.current = null;
		}
	}, []);

	useAutoScrollBottom(viewportRef, [vm.messages.length]);

	return (
		<ScrollArea ref={setScrollRoot} className="flex-1">
			{vm.messages.length === 0 ? (
				<div className="flex h-full items-center justify-center p-8">
					<Empty>
						<p className="text-sm text-muted-foreground">No messages yet.</p>
					</Empty>
				</div>
			) : (
				<div className="flex flex-col gap-1 py-2">
					{vm.messages.map((message) => (
						<MessageRow key={message.messageId} message={message} />
					))}
				</div>
			)}
		</ScrollArea>
	);
}
