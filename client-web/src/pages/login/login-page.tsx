import { LoginForm } from "./login-form";
import { LoginImage } from "./login-image";
import { useLoginViewModel } from "./use-login-view-model";

export function LoginPage() {
	const vm = useLoginViewModel();
	return (
		<div className="grid min-h-screen lg:grid-cols-2">
			<LoginForm vm={vm} />
			<LoginImage />
		</div>
	);
}
