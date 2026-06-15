import { act, renderHook, waitFor } from '@testing-library/react-native';

import { useLiveStatus } from '@/hooks/useLiveStatus';
import { MOCK_BASE_URL, MOCK_TOKEN } from '@/mocks/handlers';
import { FakeWebSocket, installFakeWebSocket } from '@/test-utils/fakeWebSocket';

const sampleStatus = {
  tempC: 24.5,
  tempValid: true,
  alerts: 0,
  timeSource: 0,
  wifiState: 2,
  minuteOfDay: 600,
  channels: [true, false],
};

describe('useLiveStatus', () => {
  let restoreWs: () => void;

  beforeEach(() => {
    restoreWs = installFakeWebSocket();
  });

  afterEach(() => {
    restoreWs();
  });

  it('starts in connecting state and targets the ws:// url', () => {
    const { result } = renderHook(() => useLiveStatus(MOCK_BASE_URL, MOCK_TOKEN));
    expect(result.current.source).toBe('connecting');
    expect(FakeWebSocket.last?.url).toBe(`${MOCK_BASE_URL.replace('http', 'ws')}/api/v1/ws`);
  });

  it('applies a websocket status frame', async () => {
    const { result } = renderHook(() => useLiveStatus(MOCK_BASE_URL, MOCK_TOKEN));
    act(() => {
      FakeWebSocket.last?.emit(sampleStatus);
    });
    await waitFor(() => {
      expect(result.current.source).toBe('websocket');
      expect(result.current.status?.tempC).toBe(24.5);
    });
  });

  it('falls back to polling on socket error and reads REST status', async () => {
    const { result } = renderHook(() => useLiveStatus(MOCK_BASE_URL, MOCK_TOKEN));
    act(() => {
      FakeWebSocket.last?.fail();
    });
    await waitFor(() => {
      expect(result.current.source).toBe('polling');
      expect(result.current.status?.channels).toHaveLength(4);
    });
  });

  it('is offline when no connection is configured', () => {
    const { result } = renderHook(() => useLiveStatus(null, null));
    expect(result.current.source).toBe('offline');
  });
});
