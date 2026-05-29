import { devices, defineConfig } from '@playwright/test';

export default defineConfig({
	testDir: './tests/e2e',
	reporter: [
		['html', { open: 'never' }],
		['list'],
	],
	outputDir: 'test-results',
	use: {
		baseURL: 'http://localhost:3000',
	},
	webServer: {
		command: 'bun --bun run dev',
		port: 3000,
		reuseExistingServer: true,
		timeout: 60_000,
	},
	projects: [
		{
			name: 'chromium',
			use: { ...devices['Desktop Chrome'] },
		},
		{
			name: 'mobile-chromium',
			use: { ...devices['Pixel 7'] },
		},
	],
});
