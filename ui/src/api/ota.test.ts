import { validateOtaMetadata } from '@/api/ota';

const validSha = 'a'.repeat(64);
// ECDSA-P256 DER signature, base64 — realistically ~96 chars.
const validSig = 'MEUCIQD' + 'a'.repeat(80) + '+/ab=';

describe('validateOtaMetadata', () => {
  it('accepts well-formed metadata', () => {
    const result = validateOtaMetadata({
      version: '2.1.0',
      sha256: validSha,
      signature: validSig,
    });
    expect(result.ok).toBe(true);
  });

  it('accepts a semver prerelease', () => {
    expect(
      validateOtaMetadata({ version: '2.1.0-rc.1', sha256: validSha, signature: validSig }).ok,
    ).toBe(true);
  });

  it('rejects a non-semver version', () => {
    const r = validateOtaMetadata({ version: 'v2', sha256: validSha, signature: validSig });
    expect(r.ok).toBe(false);
    expect(r.errors.version).toBeDefined();
  });

  it('rejects a short / non-hex sha256', () => {
    expect(
      validateOtaMetadata({ version: '2.1.0', sha256: 'abc', signature: validSig }).errors.sha256,
    ).toBeDefined();
    expect(
      validateOtaMetadata({ version: '2.1.0', sha256: 'z'.repeat(64), signature: validSig }).errors
        .sha256,
    ).toBeDefined();
  });

  it('rejects an empty / non-base64 signature', () => {
    expect(
      validateOtaMetadata({ version: '2.1.0', sha256: validSha, signature: '' }).errors.signature,
    ).toBeDefined();
    expect(
      validateOtaMetadata({ version: '2.1.0', sha256: validSha, signature: 'not base64!!' }).errors
        .signature,
    ).toBeDefined();
  });
});
