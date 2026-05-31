import { ChannelComposer } from "./channel-composer";
import { ChannelHeader } from "./channel-header";
import { ChannelMessageList } from "./channel-message-list";
import { useChannelViewModel } from "./use-channel-view-model";

interface ChannelPageProps {
	channelId: string;
}

export function ChannelPage({ channelId }: ChannelPageProps) {
	const vm = useChannelViewModel(channelId);

	return (
		<div className="flex h-full min-h-0 flex-col">
			<ChannelHeader vm={vm} />
			<ChannelMessageList vm={vm} />
			<ChannelComposer vm={vm} />
		</div>
	);
}
