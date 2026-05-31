import { createFileRoute } from "@tanstack/react-router";

import { ChannelPage } from "@/pages/channels/channel-page";

export const Route = createFileRoute("/_authenticated/channels/$channelId")({
	component: function ChannelRouteComponent() {
		const { channelId } = Route.useParams();
		return <ChannelPage channelId={channelId} />;
	},
});
