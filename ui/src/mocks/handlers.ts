import { http, HttpResponse } from 'msw';

import { RelayPolarity, type DeviceConfig, type DeviceStatus } from '@/api/schemas';

/**
 * MSW handlers emulating a real f-tank device LAN API (REQUIREMENTS §7.1).
 *
 * Used by Jest tests and (optionally) for offline UI development with no
 * hardware. The base origin is fixed to the mock device below; tests point the
 * DeviceClient at MOCK_BASE_URL.
 */

export const MOCK_BASE_URL = 'http://device.mock';
export const MOCK_TOKEN = 'mock-token-0123456789abcdef';

function makeChannel(name: string, enabled: boolean): DeviceConfig['channels'][number] {
  return {
    name,
    enabled,
    cutOnOverTemp: name.toLowerCase().includes('heater'),
    polarity: RelayPolarity.ActiveLow,
    schedule: { inverted: false, windows: [{ start: 480, end: 1080 }] },
  };
}

let config: DeviceConfig = {
  tz: 'UTC0',
  thresholds: { lowC: 20, highC: 31, hysteresisC: 0.5 },
  channels: [
    makeChannel('Lights', true),
    makeChannel('Heater', true),
    makeChannel('CO2', false),
    makeChannel('Pump', true),
  ],
};

let status: DeviceStatus = {
  tempC: 25.4,
  tempValid: true,
  alerts: 0,
  timeSource: 1,
  wifiState: 2,
  minuteOfDay: 600,
  channels: [true, true, false, true],
};

let recording = false;
let logRecords: Array<Record<string, number>> = [
  { epoch: 0, min: 600, type: 1, arg: 0 }, // boot
  { epoch: 0, min: 601, type: 4, arg: 0, tempC: 25.4 }, // temperature
  { epoch: 0, min: 602, type: 2, arg: 0 }, // channel 0 ON
];

/** Reset mutable mock state between tests. */
export function resetMockDevice() {
  config = {
    tz: 'UTC0',
    thresholds: { lowC: 20, highC: 31, hysteresisC: 0.5 },
    channels: [
      makeChannel('Lights', true),
      makeChannel('Heater', true),
      makeChannel('CO2', false),
      makeChannel('Pump', true),
    ],
  };
  status = {
    tempC: 25.4,
    tempValid: true,
    alerts: 0,
    timeSource: 1,
    wifiState: 2,
    minuteOfDay: 600,
    channels: [true, true, false, true],
  };
  recording = false;
  logRecords = [
    { epoch: 0, min: 600, type: 1, arg: 0 },
    { epoch: 0, min: 601, type: 4, arg: 0, tempC: 25.4 },
    { epoch: 0, min: 602, type: 2, arg: 0 },
  ];
}

function requireAuth(request: Request): Response | null {
  const auth = request.headers.get('Authorization');
  if (auth !== `Bearer ${MOCK_TOKEN}`) {
    return HttpResponse.json({ error: 'unauthorized' }, { status: 401 });
  }
  return null;
}

export const handlers = [
  http.get(`${MOCK_BASE_URL}/healthz`, () => HttpResponse.json({ ok: true })),

  http.get(`${MOCK_BASE_URL}/api/v1/info`, ({ request }) => {
    const unauth = requireAuth(request);
    if (unauth) return unauth;
    return HttpResponse.json({
      firmware: '2.0.0-dev',
      configVersion: 2,
      channelCount: config.channels.length,
    });
  }),

  http.get(`${MOCK_BASE_URL}/api/v1/status`, ({ request }) => {
    const unauth = requireAuth(request);
    if (unauth) return unauth;
    return HttpResponse.json(status);
  }),

  http.get(`${MOCK_BASE_URL}/api/v1/config`, ({ request }) => {
    const unauth = requireAuth(request);
    if (unauth) return unauth;
    return HttpResponse.json(config);
  }),

  http.put(`${MOCK_BASE_URL}/api/v1/config`, async ({ request }) => {
    const unauth = requireAuth(request);
    if (unauth) return unauth;
    const body = (await request.json()) as Partial<DeviceConfig>;
    if (body.channels && body.channels.length !== config.channels.length) {
      return HttpResponse.json({ error: 'channel count mismatch' }, { status: 400 });
    }
    config = { ...config, ...body } as DeviceConfig;
    return HttpResponse.json({ status: 'accepted' }, { status: 202 });
  }),

  http.post(`${MOCK_BASE_URL}/api/v1/reset`, async ({ request }) => {
    const unauth = requireAuth(request);
    if (unauth) return unauth;
    const body = (await request.json()) as { confirm?: boolean; scope?: string };
    if (!body.confirm) {
      return HttpResponse.json({ error: 'confirm required' }, { status: 400 });
    }
    if (body.scope !== 'wifi' && body.scope !== 'factory') {
      return HttpResponse.json({ error: 'invalid scope' }, { status: 400 });
    }
    return HttpResponse.json({ status: 'resetting' }, { status: 202 });
  }),

  http.get(`${MOCK_BASE_URL}/api/v1/logs`, ({ request }) => {
    const unauth = requireAuth(request);
    if (unauth) return unauth;
    return HttpResponse.json({
      recording,
      persisted: 0,
      count: logRecords.length,
      records: logRecords,
    });
  }),

  http.post(`${MOCK_BASE_URL}/api/v1/logs/recording`, async ({ request }) => {
    const unauth = requireAuth(request);
    if (unauth) return unauth;
    const body = (await request.json()) as { enabled?: boolean; clear?: boolean };
    if (body.clear) {
      logRecords = [];
    }
    if (typeof body.enabled === 'boolean') {
      recording = body.enabled;
    }
    return HttpResponse.json({ recording });
  }),

  http.post(`${MOCK_BASE_URL}/api/v1/ota`, ({ request }) => {
    const unauth = requireAuth(request);
    if (unauth) return unauth;
    // The mock accepts any image whose required headers are present.
    const sha = request.headers.get('X-FW-SHA256');
    const sig = request.headers.get('X-FW-Signature');
    const ver = request.headers.get('X-FW-Version');
    if (!sha || !sig || !ver) {
      return HttpResponse.json({ error: 'missing ota headers' }, { status: 400 });
    }
    return HttpResponse.json({ status: 'verified', reboot: true });
  }),
];
