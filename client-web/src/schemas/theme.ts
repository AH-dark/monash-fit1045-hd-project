import * as z from "zod";

export const ThemeModeSchema = z.enum(["light", "dark", "system"]);
export type ThemeModeValue = z.infer<typeof ThemeModeSchema>;
