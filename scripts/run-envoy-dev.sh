#!/usr/bin/env bash
set -euo pipefail
if [[ ! -f certs/dev/ca.pem ]]; then
  echo "certs/dev/ca.pem missing; run bash scripts/gen-dev-certs.sh first" >&2
  exit 1
fi
docker compose -f deploy/docker-compose.dev.yml up envoy
