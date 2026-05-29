import { create } from "zustand";
import { createJSONStorage, persist } from "zustand/middleware";

import type { ThemeModeValue } from "@/schemas/theme";

type ThemeState = { mode: ThemeModeValue };
type ThemeActions = { setMode: (mode: ThemeModeValue) => void };

export const useThemeStore = create<ThemeState & ThemeActions>()(
	persist(
		(set) => ({
			mode: "system" as ThemeModeValue,
			setMode: (mode) => set({ mode }),
		}),
		{
			name: "bcmd-theme",
			storage: createJSONStorage(() => localStorage),
			partialize: (s) => ({ mode: s.mode }),
		},
	),
);
