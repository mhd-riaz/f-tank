#!/usr/bin/env bash
#
# sign_firmware.sh — produce the OTA upload metadata for a built image.
#
# Computes the SHA-256 of a firmware .bin and signs it with the OTA ECDSA P-256
# PRIVATE key (which MUST stay offline — never commit it). The matching PUBLIC
# key is embedded in src/config/ota_pubkey.h.
#
# Usage:
#   scripts/sign_firmware.sh <firmware.bin> <ota_priv.pem>
#
# Prints the SHA-256 and DER signature as hex, ready for the OTA upload headers:
#   X-FW-SHA256:    <sha256 hex>
#   X-FW-Signature: <der signature hex>
#   X-FW-Version:   <your release version, e.g. 2.1.0>
#
# Generate a key pair once (keep the private key in your release vault):
#   openssl ecparam -name prime256v1 -genkey -noout -out ota_priv.pem
#   openssl ec -in ota_priv.pem -pubout -out src/config/ota_pub.pem
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <firmware.bin> <ota_priv.pem>" >&2
  exit 1
fi

BIN="$1"
KEY="$2"

if [[ ! -f "$BIN" ]]; then echo "error: firmware not found: $BIN" >&2; exit 1; fi
if [[ ! -f "$KEY" ]]; then echo "error: private key not found: $KEY" >&2; exit 1; fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# 1) SHA-256 of the raw image.
SHA_HEX="$(openssl dgst -sha256 -binary "$BIN" | xxd -p -c 256)"

# 2) ECDSA signature (DER) over that SHA-256 digest, emitted as hex.
openssl dgst -sha256 -sign "$KEY" -out "$WORK/sig.der" "$BIN"
SIG_HEX="$(xxd -p -c 256 "$WORK/sig.der")"

echo "X-FW-SHA256:    ${SHA_HEX}"
echo "X-FW-Signature: ${SIG_HEX}"
echo "size_bytes:     $(wc -c < "$BIN" | tr -d ' ')"
