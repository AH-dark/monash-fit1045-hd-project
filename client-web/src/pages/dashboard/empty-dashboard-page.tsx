import { useState } from "react";

import { MessageSquare } from "lucide-react";

import { CreateChannelDialog } from "@/components/create-channel-dialog";
import { Button } from "@/components/ui/button";
import {
	Empty,
	EmptyContent,
	EmptyDescription,
	EmptyHeader,
	EmptyMedia,
	EmptyTitle,
} from "@/components/ui/empty";

export function EmptyDashboardPage() {
	const [dialogOpen, setDialogOpen] = useState(false);

	return (
		<>
			<Empty className="min-h-[60vh] border-none">
				<EmptyHeader>
					<EmptyMedia variant="icon">
						<MessageSquare />
					</EmptyMedia>
					<EmptyTitle>Welcome to bcmd</EmptyTitle>
					<EmptyDescription>
						Select a channel from the sidebar to start chatting, or create a new
						one.
					</EmptyDescription>
				</EmptyHeader>
				<EmptyContent>
					<Button onClick={() => setDialogOpen(true)}>Create channel</Button>
				</EmptyContent>
			</Empty>
			<CreateChannelDialog open={dialogOpen} onOpenChange={setDialogOpen} />
		</>
	);
}
