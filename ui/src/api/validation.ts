import { CHANNEL_NAME_MAX, TZ_STRING_MAX, thresholdsSchema, type Thresholds } from './schemas';

/**
 * Lightweight, synchronous validators for form fields, mirroring the firmware
 * bounds (ApiValidation.h). These give instant inline feedback before a PUT;
 * the device re-validates as defense-in-depth (NFR-8).
 */

const PRINTABLE_ASCII = /^[\x20-\x7e]+$/;

export function isValidChannelName(name: string): boolean {
  return name.length >= 1 && name.length <= CHANNEL_NAME_MAX && PRINTABLE_ASCII.test(name);
}

export function isValidTz(tz: string): boolean {
  return tz.length >= 1 && tz.length <= TZ_STRING_MAX && PRINTABLE_ASCII.test(tz);
}

export interface ThresholdValidation {
  ok: boolean;
  error?: string;
}

/** Validate temperature thresholds via the shared Zod schema. */
export function validateThresholds(t: Thresholds): ThresholdValidation {
  const result = thresholdsSchema.safeParse(t);
  if (result.success) {
    return { ok: true };
  }
  return { ok: false, error: result.error.issues[0]?.message ?? 'Invalid thresholds' };
}
