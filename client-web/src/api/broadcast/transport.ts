// CRITICAL: We use createGrpcWebTransport because the C++ server speaks raw gRPC over HTTP/2.
// Envoy translates between browser gRPC-Web and server gRPC. Do NOT use createConnectTransport —
// it speaks the Connect protocol which our C++ server does not understand.
import { createGrpcWebTransport } from "@connectrpc/connect-web";

import { env } from "@/env";

export const transport = createGrpcWebTransport({
	baseUrl: env.VITE_GRPC_WEB_URL,
});
