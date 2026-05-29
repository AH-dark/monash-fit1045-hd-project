import { useEffect } from "react";

import { useShallow } from "zustand/react/shallow";

import { useThemeStore } from "@/stores/theme-store";

export function ThemeProvider({ children }: { children: React.ReactNode }) {
	const mode = useThemeStore(useShallow((s) => s.mode));

	useEffect(() => {
		const root = document.documentElement;

		function applyTheme(isDark: boolean) {
			root.classList.remove("light", "dark");
			root.classList.add(isDark ? "dark" : "light");
		}

		if (mode === "system") {
			const mq = window.matchMedia("(prefers-color-scheme: dark)");
			applyTheme(mq.matches);
			const handler = (e: MediaQueryListEvent) => applyTheme(e.matches);
			mq.addEventListener("change", handler);
			return () => mq.removeEventListener("change", handler);
		}

		applyTheme(mode === "dark");
	}, [mode]);

	return <>{children}</>;
}
