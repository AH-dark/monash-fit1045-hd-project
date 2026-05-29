import { Link } from "@tanstack/react-router";
import { FileQuestion } from "lucide-react";

import { Button } from "@/components/ui/button";

export function NotFoundPage() {
	return (
		<div className="flex min-h-screen items-center justify-center">
			<div className="space-y-4 text-center">
				<FileQuestion className="mx-auto h-16 w-16 text-muted-foreground" />
				<h1 className="text-6xl font-bold">404</h1>
				<p className="text-xl text-muted-foreground">Page not found</p>
				<p className="text-muted-foreground">
					The page you&apos;re looking for doesn&apos;t exist.
				</p>
				<Button asChild>
					<Link to={"/" as never}>Go home</Link>
				</Button>
			</div>
		</div>
	);
}
