import { DeviceClient, DeviceApiError } from '@/api/client';
import { MOCK_BASE_URL, MOCK_TOKEN } from '@/mocks/handlers';

function makeClient(token: string | undefined = MOCK_TOKEN) {
  return new DeviceClient({ baseUrl: MOCK_BASE_URL, token });
}

describe('DeviceClient against the mock device', () => {
  it('probes /healthz without auth', async () => {
    const client = new DeviceClient({ baseUrl: MOCK_BASE_URL });
    await expect(client.health()).resolves.toEqual({ ok: true });
  });

  it('fetches and validates info', async () => {
    const info = await makeClient().getInfo();
    expect(info.firmware).toBe('2.0.0-dev');
    expect(info.channelCount).toBe(4);
  });

  it('fetches and validates status', async () => {
    const status = await makeClient().getStatus();
    expect(status.tempValid).toBe(true);
    expect(status.channels).toHaveLength(4);
  });

  it('fetches and validates config', async () => {
    const config = await makeClient().getConfig();
    expect(config.channels[0].name).toBe('Lights');
    // Read-only provisioned fields are present.
    expect(config.channels[1].cutOnOverTemp).toBe(true);
  });

  it('rejects requests without a token (401)', async () => {
    const client = new DeviceClient({ baseUrl: MOCK_BASE_URL });
    await expect(client.getInfo()).rejects.toBeInstanceOf(DeviceApiError);
  });

  it('surfaces a 401 from a bad token', async () => {
    const client = makeClient('wrong-token');
    await expect(client.getStatus()).rejects.toMatchObject({ status: 401 });
  });

  it('PUTs a full channel array successfully', async () => {
    const client = makeClient();
    const config = await client.getConfig();
    await expect(client.putConfig({ channels: config.channels })).resolves.toBeUndefined();
  });

  it('rejects a partial channel array (count mismatch)', async () => {
    const client = makeClient();
    const config = await client.getConfig();
    await expect(client.putConfig({ channels: [config.channels[0]] })).rejects.toMatchObject({
      status: 400,
    });
  });

  it('performs a confirmed reset', async () => {
    await expect(makeClient().reset('wifi')).resolves.toBeUndefined();
  });

  it('fetches on-demand logs', async () => {
    const logs = await makeClient().getLogs();
    expect(logs.recording).toBe(false);
    expect(logs.records.length).toBeGreaterThan(0);
    expect(typeof logs.records[0].type).toBe('number');
  });

  it('toggles recording and clears the buffer', async () => {
    const client = makeClient();
    await client.setRecording({ enabled: true });
    let logs = await client.getLogs();
    expect(logs.recording).toBe(true);

    await client.setRecording({ clear: true });
    logs = await client.getLogs();
    expect(logs.records).toHaveLength(0);
  });

  it('uploads an OTA image with the required headers', async () => {
    const client = makeClient();
    const body = new Uint8Array([1, 2, 3, 4]).buffer;
    await expect(
      client.uploadOta(body, {
        version: '2.1.0',
        sha256: 'a'.repeat(64),
        signature: 'MEUCIQDexampleSignatureBase64Value+/abc=',
      }),
    ).resolves.toBeUndefined();
  });

  it('rejects an OTA upload missing metadata headers', async () => {
    const client = makeClient();
    const body = new Uint8Array([1, 2, 3, 4]).buffer;
    await expect(
      client.uploadOta(body, { version: '', sha256: '', signature: '' }),
    ).rejects.toMatchObject({ status: 400 });
  });
});
