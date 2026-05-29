import { expect, test } from '@playwright/test';

test('dev server responds', async ({ page }) => {
	const response = await page.goto('/');
	expect(response?.status()).toBeLessThan(500);
});
