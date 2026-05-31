import type { ReactNode } from "react";

import { Monitor, Moon, Sun } from "lucide-react";
import { useShallow } from "zustand/react/shallow";

import {
	DropdownMenuRadioGroup,
	DropdownMenuRadioItem,
} from "@/components/ui/dropdown-menu";
import type { ThemeModeValue } from "@/schemas/theme";
import { useThemeStore } from "@/stores/theme-store";

const MODES: { value: ThemeModeValue; label: string; icon: ReactNode }[] = [
	{ value: "light", label: "Light", icon: <Sun className="h-4 w-4" /> },
	{ value: "dark", label: "Dark", icon: <Moon className="h-4 w-4" /> },
	{ value: "system", label: "System", icon: <Monitor className="h-4 w-4" /> },
];

interface ThemeSwitcherProps {
	variant?: "menu" | "standalone";
}

export function ThemeSwitcher({ variant = "menu" }: ThemeSwitcherProps) {
	const { mode, setMode } = useThemeStore(
		useShallow((s) => ({ mode: s.mode, setMode: s.setMode })),
	);

	if (variant === "standalone") {
		return (
			<div className="flex gap-2">
				{MODES.map((m) => (
					<button
						key={m.value}
						type="button"
						onClick={() => setMode(m.value)}
						className={`flex items-center gap-2 rounded-md px-3 py-2 text-sm transition-colors hover:bg-accent ${
							mode === m.value ? "bg-accent font-medium" : ""
						}`}
					>
						{m.icon}
						{m.label}
					</button>
				))}
			</div>
		);
	}

	return (
		<DropdownMenuRadioGroup
			value={mode}
			onValueChange={(v) => setMode(v as ThemeModeValue)}
		>
			{MODES.map((m) => (
				<DropdownMenuRadioItem
					key={m.value}
					value={m.value}
					className="flex items-center gap-2"
				>
					{m.icon}
					{m.label}
				</DropdownMenuRadioItem>
			))}
		</DropdownMenuRadioGroup>
	);
}
