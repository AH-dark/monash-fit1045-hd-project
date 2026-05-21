#!/usr/bin/env bash
set -euo pipefail

CERTS_DIR="$(cd "$(dirname "$0")/.." && pwd)/certs/dev"
mkdir -p "$CERTS_DIR"

openssl genrsa -out "$CERTS_DIR/ca.key" 4096
openssl req -new -x509 -days 3650 -key "$CERTS_DIR/ca.key" \
  -subj "/CN=BcmdDevCA/O=bcmd-dev" -out "$CERTS_DIR/ca.pem"

openssl genrsa -out "$CERTS_DIR/server.key" 2048
openssl req -new -key "$CERTS_DIR/server.key" \
  -subj "/CN=localhost/O=bcmd-dev" -out "$CERTS_DIR/server.csr"

openssl x509 -req -days 365 \
  -in "$CERTS_DIR/server.csr" \
  -CA "$CERTS_DIR/ca.pem" -CAkey "$CERTS_DIR/ca.key" \
  -CAcreateserial \
  -extfile <(printf "subjectAltName=DNS:localhost,IP:127.0.0.1") \
  -out "$CERTS_DIR/server.pem"

rm -f "$CERTS_DIR/server.csr" "$CERTS_DIR/ca.key" "$CERTS_DIR/ca.srl"

echo "Dev certs generated in $CERTS_DIR"
