import { DeviceClient } from './client';
import type { DeviceInfo } from './schemas';

/**
 * LAN discovery + connection probing.
 *
 * The firmware does NOT yet advertise mDNS, so the primary path is manual host
 * entry (IP shown on the device OLED). `normalizeBaseUrl` accepts forgiving
 * input and produces a safe `http://host[:port]` origin; `probeDevice` confirms
 * reachability (`/healthz`) and validates the bearer token (`/api/v1/info`).
 *
 * When firmware mDNS lands, an `ftank.local` candidate can be fed straight into
 * `probeDevice` — no other change required.
 */

/** Host must be an IPv4 address or a DNS label set (e.g. `ftank.local`). */
const HOST_RE = /^(?:\d{1,3}(?:\.\d{1,3}){3}|[a-z0-9]([a-z0-9-]*[a-z0-9])?(?:\.[a-z0-9-]+)*)$/i;

export interface NormalizeResult {
  ok: boolean;
  baseUrl?: string;
  error?: string;
}

/**
 * Normalize user-entered host/URL into a clean `http://host[:port]` origin.
 * Accepts `192.168.1.42`, `192.168.1.42:8080`, `http://192.168.1.42`,
 * `ftank.local`. Rejects anything with a path, credentials, or invalid host.
 */
export function normalizeBaseUrl(input: string): NormalizeResult {
  const trimmed = input.trim();
  if (trimmed.length === 0) {
    return { ok: false, error: 'Enter the device IP address' };
  }

  // Reject embedded credentials or paths to keep the origin clean and safe.
  let working = trimmed;
  let scheme = 'http';
  const schemeMatch = /^([a-z][a-z0-9+.-]*):\/\//i.exec(working);
  if (schemeMatch) {
    const s = schemeMatch[1].toLowerCase();
    if (s !== 'http' && s !== 'https') {
      return { ok: false, error: 'Only http:// is supported on the LAN' };
    }
    scheme = s;
    working = working.slice(schemeMatch[0].length);
  }

  if (working.includes('@')) {
    return { ok: false, error: 'Credentials in the address are not allowed' };
  }
  // Strip any path/query/fragment — we only want the origin.
  working = working.split(/[/?#]/, 1)[0];

  let host = working;
  let port: string | undefined;
  const lastColon = working.lastIndexOf(':');
  if (lastColon !== -1) {
    host = working.slice(0, lastColon);
    port = working.slice(lastColon + 1);
    if (!/^\d{1,5}$/.test(port) || Number(port) < 1 || Number(port) > 65535) {
      return { ok: false, error: 'Invalid port' };
    }
  }

  if (!HOST_RE.test(host)) {
    return { ok: false, error: 'Invalid IP address or hostname' };
  }
  // Bound each IPv4 octet to 0–255.
  if (/^\d{1,3}(\.\d{1,3}){3}$/.test(host)) {
    const octets = host.split('.').map(Number);
    if (octets.some((o) => o > 255)) {
      return { ok: false, error: 'Invalid IP address' };
    }
  }

  const baseUrl = port ? `${scheme}://${host}:${port}` : `${scheme}://${host}`;
  return { ok: true, baseUrl };
}

export type ProbeOutcome =
  | { status: 'ok'; info: DeviceInfo }
  | { status: 'unreachable' }
  | { status: 'unauthorized' }
  | { status: 'not-ftank' }
  | { status: 'error'; message: string };

/**
 * Probe a normalized base URL: confirm a device responds on `/healthz`, then
 * verify the bearer token via `/api/v1/info`. Returns a discriminated outcome
 * so the UI can show a precise message.
 */
export async function probeDevice(
  baseUrl: string,
  token: string,
  fetchImpl: typeof fetch = fetch,
): Promise<ProbeOutcome> {
  const probe = new DeviceClient({ baseUrl, fetchImpl, timeoutMs: 5000 });
  try {
    const health = await probe.health();
    if (!health.ok) {
      return { status: 'not-ftank' };
    }
  } catch {
    return { status: 'unreachable' };
  }

  const authed = new DeviceClient({ baseUrl, token, fetchImpl, timeoutMs: 5000 });
  try {
    const info = await authed.getInfo();
    return { status: 'ok', info };
  } catch (err) {
    const status = (err as { status?: number }).status;
    if (status === 401) {
      return { status: 'unauthorized' };
    }
    return { status: 'error', message: (err as Error).message ?? 'Could not reach device' };
  }
}
