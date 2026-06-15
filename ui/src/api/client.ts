import {
  deviceConfigSchema,
  healthSchema,
  infoSchema,
  logsResponseSchema,
  statusSchema,
  type DeviceConfig,
  type DeviceInfo,
  type DeviceStatus,
  type LogsResponse,
} from './schemas';
import type { OtaMetadata } from './ota';

/**
 * Typed client for the f-tank device LAN API (REQUIREMENTS §7.1).
 *
 * Plain HTTP on the LAN (AD-4); every `/api/v1/*` call carries the per-device
 * bearer token issued at BLE provisioning. Responses are validated with Zod so
 * malformed device output is caught at the boundary, not deep in the UI.
 */

/** Reset scopes accepted by `POST /api/v1/reset` (FR-10). */
export type ResetScope = 'wifi' | 'factory';

/** Structured error thrown for any non-2xx device response. */
export class DeviceApiError extends Error {
  constructor(
    readonly status: number,
    message: string,
  ) {
    super(message);
    this.name = 'DeviceApiError';
  }
}

export interface DeviceClientOptions {
  /** Base origin, e.g. `http://192.168.1.42` (no trailing slash). */
  baseUrl: string;
  /** Per-device bearer token; omitted only for the unauthenticated probe. */
  token?: string;
  /** Injectable for tests; defaults to global fetch. */
  fetchImpl?: typeof fetch;
  /** Per-request timeout (ms). */
  timeoutMs?: number;
}

export class DeviceClient {
  private readonly baseUrl: string;
  private readonly token?: string;
  private readonly fetchImpl: typeof fetch;
  private readonly timeoutMs: number;

  constructor(opts: DeviceClientOptions) {
    this.baseUrl = opts.baseUrl.replace(/\/+$/, '');
    this.token = opts.token;
    this.fetchImpl = opts.fetchImpl ?? fetch;
    this.timeoutMs = opts.timeoutMs ?? 8000;
  }

  /** Unauthenticated liveness probe — used by discovery. */
  async health(): Promise<{ ok: boolean }> {
    const json = await this.request('GET', '/healthz', { auth: false });
    return healthSchema.parse(json);
  }

  async getInfo(): Promise<DeviceInfo> {
    return infoSchema.parse(await this.request('GET', '/api/v1/info'));
  }

  async getStatus(): Promise<DeviceStatus> {
    return statusSchema.parse(await this.request('GET', '/api/v1/status'));
  }

  async getConfig(): Promise<DeviceConfig> {
    return deviceConfigSchema.parse(await this.request('GET', '/api/v1/config'));
  }

  /**
   * Update user-editable config. The firmware overlays only known fields and
   * requires that, when `channels` is present, its length equals the device's
   * channel count — so callers send the FULL channel array (read-only polarity
   * and cut-target are preserved server-side).
   */
  async putConfig(config: Partial<DeviceConfig>): Promise<void> {
    await this.request('PUT', '/api/v1/config', { body: config });
  }

  async reset(scope: ResetScope): Promise<void> {
    await this.request('POST', '/api/v1/reset', {
      body: { confirm: true, scope },
    });
  }

  /** On-demand event log: buffered records + recording flag + persisted count. */
  async getLogs(): Promise<LogsResponse> {
    return logsResponseSchema.parse(await this.request('GET', '/api/v1/logs'));
  }

  /** Toggle on-demand recording and/or clear the buffer (FR-29). */
  async setRecording(opts: { enabled?: boolean; clear?: boolean }): Promise<void> {
    await this.request('POST', '/api/v1/logs/recording', { body: opts });
  }

  /**
   * Upload a signed firmware image (FR-38/39). The metadata travels in headers;
   * the device verifies SHA-256 + ECDSA-P256 signature + anti-rollback itself.
   * `body` is the raw image bytes (Blob/ArrayBuffer); we never sign or alter it.
   */
  async uploadOta(body: Blob | ArrayBuffer, meta: OtaMetadata): Promise<void> {
    if (!this.token) {
      throw new DeviceApiError(401, 'no device token');
    }
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 60_000);
    let res: Response;
    try {
      res = await this.fetchImpl(`${this.baseUrl}/api/v1/ota`, {
        method: 'POST',
        headers: {
          Authorization: `Bearer ${this.token}`,
          'Content-Type': 'application/octet-stream',
          'X-FW-Version': meta.version,
          'X-FW-SHA256': meta.sha256,
          'X-FW-Signature': meta.signature,
        },
        body: body as BodyInit,
        signal: controller.signal,
      });
    } finally {
      clearTimeout(timer);
    }
    if (!res.ok) {
      const text = await res.text();
      const json = text ? safeJson(text) : undefined;
      const msg =
        (json && typeof json === 'object' && 'error' in json
          ? String((json as { error: unknown }).error)
          : undefined) ?? `OTA upload failed (${res.status})`;
      throw new DeviceApiError(res.status, msg);
    }
  }

  private async request(
    method: string,
    path: string,
    opts: { auth?: boolean; body?: unknown } = {},
  ): Promise<unknown> {
    const { auth = true, body } = opts;
    const headers: Record<string, string> = { Accept: 'application/json' };
    if (auth) {
      if (!this.token) {
        throw new DeviceApiError(401, 'no device token');
      }
      headers.Authorization = `Bearer ${this.token}`;
    }
    if (body !== undefined) {
      headers['Content-Type'] = 'application/json';
    }

    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), this.timeoutMs);
    let res: Response;
    try {
      res = await this.fetchImpl(`${this.baseUrl}${path}`, {
        method,
        headers,
        body: body !== undefined ? JSON.stringify(body) : undefined,
        signal: controller.signal,
      });
    } finally {
      clearTimeout(timer);
    }

    const text = await res.text();
    const json = text ? safeJson(text) : undefined;
    if (!res.ok) {
      const msg =
        (json && typeof json === 'object' && 'error' in json
          ? String((json as { error: unknown }).error)
          : undefined) ?? `request failed (${res.status})`;
      throw new DeviceApiError(res.status, msg);
    }
    return json;
  }
}

function safeJson(text: string): unknown {
  try {
    return JSON.parse(text);
  } catch {
    return undefined;
  }
}
