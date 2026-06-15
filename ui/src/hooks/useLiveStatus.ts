import { useEffect, useRef, useState } from 'react';

import { DeviceClient } from '@/api/client';
import { statusSchema, type DeviceStatus } from '@/api/schemas';

/**
 * Live device status (FR-33, AD-3): subscribes to the firmware WebSocket
 * (`/api/v1/ws`, ~1 Hz push) and falls back to REST polling of `/api/v1/status`
 * if the socket cannot be opened or drops. The WS is unauthenticated on the
 * device (browsers/RN can't set WS headers), matching the firmware.
 */

export type LiveSource = 'connecting' | 'websocket' | 'polling' | 'offline';

export interface LiveStatus {
  status: DeviceStatus | null;
  source: LiveSource;
  /** Wall-clock ms of the last successful update, or null. */
  lastUpdate: number | null;
}

const POLL_INTERVAL_MS = 2000;
const WS_STALE_MS = 4000;

function toWebSocketUrl(baseUrl: string): string {
  return `${baseUrl.replace(/^http/i, 'ws')}/api/v1/ws`;
}

export function useLiveStatus(baseUrl: string | null, token: string | null): LiveStatus {
  const [status, setStatus] = useState<DeviceStatus | null>(null);
  const [source, setSource] = useState<LiveSource>('connecting');
  const [lastUpdate, setLastUpdate] = useState<number | null>(null);

  // Keep the latest update time in a ref for the staleness check.
  const lastUpdateRef = useRef<number | null>(null);
  lastUpdateRef.current = lastUpdate;

  useEffect(() => {
    if (!baseUrl || !token) {
      setSource('offline');
      return;
    }

    let cancelled = false;
    let ws: WebSocket | null = null;
    let pollTimer: ReturnType<typeof setInterval> | null = null;
    let staleTimer: ReturnType<typeof setInterval> | null = null;

    const apply = (raw: unknown, via: LiveSource) => {
      const parsed = statusSchema.safeParse(raw);
      if (!parsed.success || cancelled) {
        return;
      }
      setStatus(parsed.data);
      setSource(via);
      setLastUpdate(Date.now());
    };

    const startPolling = () => {
      if (pollTimer) {
        return;
      }
      const client = new DeviceClient({ baseUrl, token, timeoutMs: POLL_INTERVAL_MS });
      const tick = async () => {
        try {
          const s = await client.getStatus();
          apply(s, 'polling');
        } catch {
          if (!cancelled) {
            setSource('offline');
          }
        }
      };
      void tick();
      pollTimer = setInterval(tick, POLL_INTERVAL_MS);
    };

    const stopPolling = () => {
      if (pollTimer) {
        clearInterval(pollTimer);
        pollTimer = null;
      }
    };

    try {
      ws = new WebSocket(toWebSocketUrl(baseUrl));
      ws.onmessage = (event: WebSocketMessageEvent) => {
        stopPolling();
        try {
          apply(JSON.parse(String(event.data)), 'websocket');
        } catch {
          /* ignore malformed frames */
        }
      };
      ws.onerror = () => startPolling();
      ws.onclose = () => {
        if (!cancelled) {
          startPolling();
        }
      };
    } catch {
      startPolling();
    }

    // If the socket goes quiet, fall back to polling until it recovers.
    staleTimer = setInterval(() => {
      const last = lastUpdateRef.current;
      if (last !== null && Date.now() - last > WS_STALE_MS) {
        startPolling();
      }
    }, WS_STALE_MS);

    return () => {
      cancelled = true;
      stopPolling();
      if (staleTimer) {
        clearInterval(staleTimer);
      }
      if (ws) {
        ws.onmessage = null;
        ws.onerror = null;
        ws.onclose = null;
        ws.close();
      }
    };
  }, [baseUrl, token]);

  return { status, source, lastUpdate };
}
