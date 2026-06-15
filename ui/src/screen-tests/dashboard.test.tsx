import { act, screen, waitFor } from '@testing-library/react-native';
import { renderRouter } from 'expo-router/testing-library';
import { QueryClientProvider } from '@tanstack/react-query';
import { Text } from 'react-native';

import { queryClient } from '@/api/queryClient';
import DashboardScreen from '@/app/index';
import { MOCK_BASE_URL, MOCK_TOKEN } from '@/mocks/handlers';
import { useConnectionStore } from '@/store/connection';
import { FakeWebSocket, installFakeWebSocket } from '@/test-utils/fakeWebSocket';

/**
 * Render + behavior test for the Dashboard. Proves the real screen mounts,
 * redirects to /connect when unpaired, and renders live data from a WebSocket
 * frame when paired. A controllable fake WebSocket makes the live path
 * deterministic (no reliance on polling-fallback timing).
 */
function Dashboard() {
  return (
    <QueryClientProvider client={queryClient}>
      <DashboardScreen />
    </QueryClientProvider>
  );
}

function ConnectMarker() {
  return <Text testID="connect-route">connect</Text>;
}

const liveStatus = {
  tempC: 26.7,
  tempValid: true,
  alerts: 0,
  timeSource: 0,
  wifiState: 2,
  minuteOfDay: 600,
  channels: [true, false, true, false],
};

describe('DashboardScreen (render + redirect)', () => {
  let restoreWs: () => void;

  beforeEach(() => {
    restoreWs = installFakeWebSocket();
  });

  afterEach(async () => {
    restoreWs();
    await useConnectionStore.getState().clear();
  });

  it('redirects to /connect when no device is paired', async () => {
    await useConnectionStore.getState().clear();
    useConnectionStore.setState({ hydrated: true });

    const { unmount } = renderRouter({ index: Dashboard, connect: ConnectMarker });

    await waitFor(() => {
      expect(screen.getByTestId('connect-route')).toBeTruthy();
    });
    await act(async () => {
      unmount();
    });
  });

  it('renders live temperature from a websocket frame when paired', async () => {
    await useConnectionStore.getState().setConnection(MOCK_BASE_URL, MOCK_TOKEN);
    useConnectionStore.setState({ hydrated: true });

    const { unmount } = renderRouter({ index: Dashboard, connect: ConnectMarker });

    // Drive a live frame deterministically through the fake socket.
    await act(async () => {
      FakeWebSocket.last?.emit(liveStatus);
    });

    await waitFor(() => {
      expect(screen.getByText('26.7°C')).toBeTruthy();
    });
    await act(async () => {
      unmount();
    });
  });
});
