import {
	useCallback,
	useEffect,
	useLayoutEffect,
	useRef,
	useState,
} from "react";

import { useVirtualizer } from "@tanstack/react-virtual";
import { Loader2 } from "lucide-react";

import { Avatar, AvatarFallback } from "@/components/ui/avatar";
import { Empty } from "@/components/ui/empty";
import type { Message } from "@/schemas/message";

import type { ChannelViewModel } from "./use-channel-view-model";

interface ChannelMessageListProps {
	vm: ChannelViewModel;
}

const ESTIMATED_ROW_HEIGHT = 64;
const LOAD_MORE_TRIGGER_INDEX = 3;
const STICK_TO_BOTTOM_THRESHOLD_PX = 24;

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
	const [viewportEl, setViewportEl] = useState<HTMLDivElement | null>(null);
	const setViewport = useCallback((node: HTMLDivElement | null) => {
		setViewportEl(node);
	}, []);

	const messages = vm.messages;
	const hasMoreHistory = vm.hasMoreHistory;
	const isLoadingHistory = vm.isLoadingHistory;
	const loadOlderHistory = vm.loadOlderHistory;

	const virtualizer = useVirtualizer({
		count: messages.length,
		getScrollElement: () => viewportEl,
		estimateSize: () => ESTIMATED_ROW_HEIGHT,
		overscan: 8,
		getItemKey: (index) => messages[index].messageId,
	});

	const virtualItems = virtualizer.getVirtualItems();
	const totalSize = virtualizer.getTotalSize();

	const isAtBottomRef = useRef(true);
	const prevCountRef = useRef(0);

	useEffect(() => {
		if (!viewportEl) return;
		const update = () => {
			const distance =
				viewportEl.scrollHeight -
				viewportEl.scrollTop -
				viewportEl.clientHeight;
			isAtBottomRef.current = distance <= STICK_TO_BOTTOM_THRESHOLD_PX;
		};
		update();
		viewportEl.addEventListener("scroll", update, { passive: true });
		return () => viewportEl.removeEventListener("scroll", update);
	}, [viewportEl]);

	useLayoutEffect(() => {
		if (!viewportEl) return;
		const count = messages.length;
		if (count === 0) {
			prevCountRef.current = 0;
			return;
		}
		const grewAtEnd = count > prevCountRef.current;
		const isInitialLoad = prevCountRef.current === 0;

		if (isInitialLoad || (grewAtEnd && isAtBottomRef.current)) {
			const scrollToBottom = () => {
				viewportEl.scrollTop = viewportEl.scrollHeight;
			};
			scrollToBottom();
			// Second pass after virtualizer measures the new row (dynamic height).
			requestAnimationFrame(scrollToBottom);
		}

		prevCountRef.current = count;
	}, [messages, viewportEl]);

	useEffect(() => {
		if (!hasMoreHistory || isLoadingHistory) return;
		if (virtualItems.length === 0) return;
		if (virtualItems[0].index <= LOAD_MORE_TRIGGER_INDEX) {
			loadOlderHistory();
		}
	}, [virtualItems, hasMoreHistory, isLoadingHistory, loadOlderHistory]);

	if (messages.length === 0 && !isLoadingHistory) {
		return (
			<div className="flex flex-1 items-center justify-center p-8">
				<Empty>
					<p className="text-sm text-muted-foreground">No messages yet.</p>
				</Empty>
			</div>
		);
	}

	return (
		<div
			ref={setViewport}
			className="flex-1 min-h-0 overflow-y-auto"
			data-testid="channel-message-viewport"
		>
			{hasMoreHistory && (
				<div className="flex items-center justify-center py-2">
					{isLoadingHistory ? (
						<Loader2 className="h-4 w-4 animate-spin text-muted-foreground" />
					) : (
						<span className="text-xs text-muted-foreground">
							Scroll up to load older messages
						</span>
					)}
				</div>
			)}
			<div className="relative w-full" style={{ height: `${totalSize}px` }}>
				{virtualItems.map((virtualItem) => {
					const message = messages[virtualItem.index];
					if (!message) return null;
					return (
						<div
							key={virtualItem.key}
							ref={virtualizer.measureElement}
							data-index={virtualItem.index}
							className="absolute top-0 left-0 w-full"
							style={{ transform: `translateY(${virtualItem.start}px)` }}
						>
							<MessageRow message={message} />
						</div>
					);
				})}
			</div>
		</div>
	);
}
