import crypto from 'node:crypto';

export function createTestUsername() {
	return `e2e-${crypto.randomUUID().slice(0, 8)}`;
}
