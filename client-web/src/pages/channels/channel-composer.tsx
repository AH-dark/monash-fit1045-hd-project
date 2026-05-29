import type { KeyboardEvent } from "react";
import { useCallback, useState } from "react";

import { Button } from "@/components/ui/button";
import { Textarea } from "@/components/ui/textarea";
import { useIsMobile } from "@/hooks/use-mobile";
import { toastBroadcastPromise } from "@/lib/toast-helpers";

import type { ChannelViewModel } from "./use-channel-view-model";

interface ChannelComposerProps {
	vm: ChannelViewModel;
}

export function ChannelComposer({ vm }: ChannelComposerProps) {
	const [content, setContent] = useState("");
	const [isSubmitting, setIsSubmitting] = useState(false);
	const isMobile = useIsMobile();

	const canSend = content.trim().length > 0 && vm.isConnected && !isSubmitting;

	const send = useCallback(async () => {
		const parsed = vm.schema.safeParse({ content });
		if (!parsed.success) return;

		setIsSubmitting(true);
		setContent("");
		await toastBroadcastPromise(vm.sendMessage(parsed.data.content), {
			loading: "Sending...",
			success: "Sent",
		}).catch(() => undefined);
		setIsSubmitting(false);
	}, [content, vm]);

	const handleKeyDown = (event: KeyboardEvent<HTMLTextAreaElement>) => {
		if (event.key !== "Enter") return;
		if (isMobile) return;
		if (event.shiftKey) return;
		event.preventDefault();
		if (!canSend) return;
		void send();
	};

	const handleSendClick = () => {
		if (!canSend) return;
		void send();
	};

	return (
		<form
			className="flex shrink-0 items-end gap-2 border-t p-3"
			onSubmit={(event) => {
				event.preventDefault();
				if (!canSend) return;
				void send();
			}}
		>
			<Textarea
				value={content}
				onChange={(event) => setContent(event.target.value)}
				onKeyDown={handleKeyDown}
				placeholder={vm.isConnected ? "Type a message" : "Not connected"}
				disabled={!vm.isConnected || isSubmitting}
				rows={1}
				maxLength={2000}
				className="max-h-40 min-h-10 resize-none"
				aria-label="Message"
			/>
			<Button
				type="submit"
				size="sm"
				disabled={!canSend}
				onClick={handleSendClick}
			>
				Send
			</Button>
		</form>
	);
}
