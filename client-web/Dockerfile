# syntax=docker/dockerfile:1.24

# ===========================================================================
# base — Bun image used by all stages
# ===========================================================================
FROM oven/bun:1 AS base
WORKDIR /app

# ===========================================================================
# deps — install dependencies (cached by package.json + bun.lock)
# ===========================================================================
FROM base AS deps

COPY package.json bun.lock ./
RUN --mount=type=cache,target=/root/.bun/install/cache \
    bun install --frozen-lockfile

# ===========================================================================
# dev — TanStack Start dev server with hot reload (used by docker-compose)
# ===========================================================================
FROM base AS dev

COPY --from=deps /app/node_modules ./node_modules
COPY . .

EXPOSE 3000

# Vite binds to 127.0.0.1 by default; --host 0.0.0.0 exposes it to the container network.
CMD ["bunx", "--bun", "vite", "dev", "--host", "0.0.0.0", "--port", "3000"]

# ===========================================================================
# builder — produce the production .output bundle
# ===========================================================================
FROM base AS builder

COPY --from=deps /app/node_modules ./node_modules
COPY . .

RUN bun --bun run build

# ===========================================================================
# prod — production runtime (Nitro server)
# ===========================================================================
FROM oven/bun:1-slim AS prod

WORKDIR /app

COPY --from=builder /app/.output ./.output
COPY --from=builder /app/package.json ./package.json

EXPOSE 3000

ENV HOST=0.0.0.0 PORT=3000

CMD ["bun", "run", ".output/server/index.mjs"]
