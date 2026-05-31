import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";

export function AboutPage() {
	return (
		<div className="container mx-auto max-w-2xl p-6">
			<Card>
				<CardHeader>
					<CardTitle>About bcmd</CardTitle>
				</CardHeader>
				<CardContent className="space-y-4">
					<p className="text-muted-foreground">
						bcmd is a C++23 broadcast messaging system with a TanStack Start web
						client. Built for FIT1045 HD.
					</p>
					<div>
						<h3 className="mb-2 font-semibold">Tech Stack</h3>
						<ul className="list-inside list-disc space-y-1 text-sm text-muted-foreground">
							<li>TanStack Start + React 19</li>
							<li>shadcn/ui + Tailwind CSS v4</li>
							<li>ConnectRPC / gRPC-Web</li>
							<li>Zustand + TanStack Query</li>
						</ul>
					</div>
					<a
						href="https://github.com/AH-dark/monash-fit1045-hd-project"
						target="_blank"
						rel="noopener noreferrer"
						className="text-sm text-primary underline-offset-4 hover:underline"
					>
						View on GitHub
					</a>
				</CardContent>
			</Card>
		</div>
	);
}
