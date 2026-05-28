import { createClient } from "@connectrpc/connect";

import { BroadcastService } from "@/gen/bcmd/v1/broadcast_pb.ts";

import { transport } from "./transport.ts";

export const broadcastClient = createClient(BroadcastService, transport);
