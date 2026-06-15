import { http, HttpResponse } from 'msw';

import { normalizeBaseUrl, probeDevice } from '@/api/discovery';
import { MOCK_BASE_URL, MOCK_TOKEN } from '@/mocks/handlers';
import { server } from '@/mocks/server';

describe('normalizeBaseUrl', () => {
  it('accepts a bare IPv4 and defaults to http', () => {
    expect(normalizeBaseUrl('192.168.1.42')).toEqual({
      ok: true,
      baseUrl: 'http://192.168.1.42',
    });
  });

  it('accepts an IPv4 with port', () => {
    expect(normalizeBaseUrl('192.168.1.42:8080').baseUrl).toBe('http://192.168.1.42:8080');
  });

  it('accepts an http URL and strips a trailing path', () => {
    expect(normalizeBaseUrl('http://192.168.1.42/api/v1').baseUrl).toBe('http://192.168.1.42');
  });

  it('accepts an mDNS hostname', () => {
    expect(normalizeBaseUrl('ftank.local').baseUrl).toBe('http://ftank.local');
  });

  it('rejects empty input', () => {
    expect(normalizeBaseUrl('   ').ok).toBe(false);
  });

  it('rejects https (LAN is http only per AD-4)... but allows it explicitly', () => {
    // https is permitted syntactically (cloud path); only unknown schemes fail.
    expect(normalizeBaseUrl('ftp://192.168.1.42').ok).toBe(false);
  });

  it('rejects embedded credentials', () => {
    expect(normalizeBaseUrl('http://user:pass@192.168.1.42').ok).toBe(false);
  });

  it('rejects out-of-range octets', () => {
    expect(normalizeBaseUrl('999.1.1.1').ok).toBe(false);
  });

  it('rejects an invalid port', () => {
    expect(normalizeBaseUrl('192.168.1.42:70000').ok).toBe(false);
  });
});

describe('probeDevice against the mock device', () => {
  it('returns ok with info for a valid token', async () => {
    const result = await probeDevice(MOCK_BASE_URL, MOCK_TOKEN);
    expect(result.status).toBe('ok');
    if (result.status === 'ok') {
      expect(result.info.firmware).toBe('2.0.0-dev');
    }
  });

  it('returns unauthorized for a bad token', async () => {
    const result = await probeDevice(MOCK_BASE_URL, 'wrong-token');
    expect(result.status).toBe('unauthorized');
  });

  it('returns unreachable for a non-responding host', async () => {
    server.use(http.get('http://10.255.255.1/healthz', () => HttpResponse.error()));
    const result = await probeDevice('http://10.255.255.1', MOCK_TOKEN);
    expect(result.status).toBe('unreachable');
  });
});
