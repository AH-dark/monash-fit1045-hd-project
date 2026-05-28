import { beforeEach, describe, expect, it } from "vitest";
import { useAuthStore } from "#/stores/auth-store";

describe("auth-store", () => {
	beforeEach(() => {
		localStorage.clear();
		useAuthStore.setState({
			status: "idle",
			clientId: null,
			username: null,
		});
	});

	it("initial state has status 'idle' and null clientId/username", () => {
		const state = useAuthStore.getState();
		expect(state.status).toBe("idle");
		expect(state.clientId).toBeNull();
		expect(state.username).toBeNull();
	});

	it("setConnecting changes status to 'connecting'", () => {
		useAuthStore.getState().setConnecting();
		expect(useAuthStore.getState().status).toBe("connecting");
	});

	it("setConnected updates clientId, username, and status to 'connected'", () => {
		useAuthStore.getState().setConnected("client-123", "alice");
		const state = useAuthStore.getState();
		expect(state.status).toBe("connected");
		expect(state.clientId).toBe("client-123");
		expect(state.username).toBe("alice");
	});

	it("setDisconnected clears clientId and username", () => {
		useAuthStore.getState().setConnected("client-123", "alice");
		useAuthStore.getState().setDisconnected();
		const state = useAuthStore.getState();
		expect(state.status).toBe("disconnected");
		expect(state.clientId).toBeNull();
		expect(state.username).toBeNull();
	});

	it("reset clears all fields", () => {
		useAuthStore.getState().setConnected("client-123", "alice");
		useAuthStore.getState().reset();
		const state = useAuthStore.getState();
		expect(state.clientId).toBeNull();
		expect(state.username).toBeNull();
	});
});
