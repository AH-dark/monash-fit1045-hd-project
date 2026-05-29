import type { DependencyList, RefObject } from "react";
import { useCallback, useEffect, useMemo, useState } from "react";

export function useAutoScrollBottom(
	ref: RefObject<HTMLElement | null>,
	deps: DependencyList,
): { isAtBottom: boolean; scrollToBottom: () => void } {
	const [isAtBottom, setIsAtBottom] = useState(true);

	const scrollToBottom = useCallback(() => {
		if (!ref.current) return;

		ref.current.scrollTop = ref.current.scrollHeight;
	}, [ref]);

	useEffect(() => {
		if (!ref.current) return;

		const el = ref.current;
		const atBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 20;

		if (atBottom) {
			el.scrollTop = el.scrollHeight;
		}
		// biome-ignore lint/correctness/useExhaustiveDependencies: deps are passed as parameter by design
	}, deps);

	useEffect(() => {
		const el = ref.current;
		if (!el) return;

		const handler = () => {
			setIsAtBottom(el.scrollHeight - el.scrollTop - el.clientHeight < 20);
		};

		handler();
		el.addEventListener("scroll", handler);

		return () => el.removeEventListener("scroll", handler);
	}, [ref]);

	return useMemo(
		() => ({ isAtBottom, scrollToBottom }),
		[isAtBottom, scrollToBottom],
	);
}
