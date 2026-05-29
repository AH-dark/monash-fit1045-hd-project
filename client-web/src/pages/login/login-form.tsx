import { type FormEvent, useId, useState } from "react";

import { useNavigate } from "@tanstack/react-router";

import { Button } from "@/components/ui/button";
import {
	Card,
	CardContent,
	CardDescription,
	CardFooter,
	CardHeader,
	CardTitle,
} from "@/components/ui/card";
import {
	Field,
	FieldDescription,
	FieldError,
	FieldGroup,
	FieldLabel,
} from "@/components/ui/field";
import { Input } from "@/components/ui/input";
import { toastBroadcastPromise } from "@/lib/toast-helpers";

import type { LoginViewModel } from "./use-login-view-model";

export interface LoginFormProps {
	readonly vm: LoginViewModel;
}

export function LoginForm({ vm }: LoginFormProps) {
	const usernameId = useId();
	const navigate = useNavigate();
	const [username, setUsername] = useState("");
	const [validationError, setValidationError] = useState<string | null>(null);

	const handleSubmit = async (event: FormEvent<HTMLFormElement>) => {
		event.preventDefault();

		const result = vm.schema.safeParse({ username });
		if (!result.success) {
			const firstIssue = result.error.issues[0];
			setValidationError(firstIssue?.message ?? "Invalid username");
			return;
		}

		setValidationError(null);

		await toastBroadcastPromise(vm.connect(result.data.username), {
			loading: "Connecting...",
			success: "Connected!",
		})
			.then(() => navigate({ to: "/" as never }))
			.catch(() => undefined);
	};

	const hasError = validationError !== null;

	return (
		<div className="flex items-center justify-center p-6 md:p-10">
			<div className="w-full max-w-sm">
				<Card>
					<CardHeader>
						<CardTitle className="text-2xl">Welcome back</CardTitle>
						<CardDescription>
							Enter a username to connect to the broadcast server.
						</CardDescription>
					</CardHeader>
					<form onSubmit={handleSubmit} noValidate>
						<CardContent>
							<FieldGroup>
								<Field data-invalid={hasError ? true : undefined}>
									<FieldLabel htmlFor={usernameId}>Username</FieldLabel>
									<Input
										id={usernameId}
										name="username"
										type="text"
										autoComplete="username"
										autoFocus
										placeholder="alice"
										value={username}
										onChange={(event) => {
											setUsername(event.target.value);
											if (validationError) {
												setValidationError(null);
											}
										}}
										disabled={vm.isConnecting}
										aria-invalid={hasError || undefined}
									/>
									{hasError ? (
										<FieldError>{validationError}</FieldError>
									) : (
										<FieldDescription>
											Letters, numbers, _ and - only. Up to 32 characters.
										</FieldDescription>
									)}
								</Field>
							</FieldGroup>
						</CardContent>
						<CardFooter className="mt-6 flex-col gap-2">
							<Button
								type="submit"
								className="w-full"
								disabled={vm.isConnecting}
							>
								{vm.isConnecting ? "Connecting..." : "Connect"}
							</Button>
						</CardFooter>
					</form>
				</Card>
			</div>
		</div>
	);
}
