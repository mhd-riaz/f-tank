/**
 * Validation + types for local signed OTA upload (FR-38/39).
 *
 * The device verifies the image itself: it computes the SHA-256, checks it
 * against `X-FW-SHA256`, then verifies an ECDSA-P256 signature (`X-FW-Signature`)
 * over that digest against an embedded public key, and enforces anti-rollback
 * against `X-FW-Version`. The app merely relays these from the signed release
 * artifact (produced offline by scripts/sign_firmware.sh) — it never signs.
 */

export interface OtaMetadata {
  version: string;
  sha256: string;
  signature: string;
}

export interface OtaValidation {
  ok: boolean;
  errors: Partial<Record<keyof OtaMetadata, string>>;
}

const SEMVER_RE = /^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$/;
const SHA256_HEX_RE = /^[0-9a-f]{64}$/i;
const BASE64_RE = /^[A-Za-z0-9+/]+={0,2}$/;

/** Validate the metadata that travels in the OTA request headers. */
export function validateOtaMetadata(meta: OtaMetadata): OtaValidation {
  const errors: OtaValidation['errors'] = {};

  if (!SEMVER_RE.test(meta.version.trim())) {
    errors.version = 'Version must be semver (e.g. 2.1.0).';
  }
  if (!SHA256_HEX_RE.test(meta.sha256.trim())) {
    errors.sha256 = 'SHA-256 must be 64 hex characters.';
  }
  const sig = meta.signature.trim();
  // ECDSA-P256 signature, base64-encoded; bound the length to a sane range.
  if (sig.length < 64 || sig.length > 256 || !BASE64_RE.test(sig)) {
    errors.signature = 'Signature must be a base64 string.';
  }

  return { ok: Object.keys(errors).length === 0, errors };
}
