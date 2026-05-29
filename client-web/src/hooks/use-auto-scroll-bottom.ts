import { useCallback, useEffect, useMemo, useState } from "react";

import type { DependencyList, RefObject } from "react";

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
		// eslint-disable-next-line react-hooks/exhaustive-deps
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

	return useMemo(() => ({ isAtBottom, scrollToBottom }), [isAtBottom, scrollToBottom]);
}
