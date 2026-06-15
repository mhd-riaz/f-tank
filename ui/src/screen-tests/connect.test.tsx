import { act, fireEvent, screen, waitFor } from '@testing-library/react-native';
import { renderRouter } from 'expo-router/testing-library';
import { QueryClientProvider } from '@tanstack/react-query';

import { queryClient } from '@/api/queryClient';
import ConnectScreen from '@/app/connect';
import { MOCK_BASE_URL, MOCK_TOKEN } from '@/mocks/handlers';
import { useConnectionStore } from '@/store/connection';

/**
 * Render smoke + behavior test for the Connect screen. Proves the real screen
 * mounts (not just that it bundles) and that a valid host+token probes the
 * device and persists the connection. The mock device lives at MOCK_BASE_URL;
 * `normalizeBaseUrl` strips the scheme, so we drive the host field with the
 * bare host.
 */
const MOCK_HOST = MOCK_BASE_URL.replace(/^https?:\/\//, '');

function ConnectRoute() {
  return (
    <QueryClientProvider client={queryClient}>
      <ConnectScreen />
    </QueryClientProvider>
  );
}

describe('ConnectScreen (render + flow)', () => {
  beforeEach(async () => {
    await useConnectionStore.getState().clear();
  });

  it('renders the connect form', () => {
    renderRouter({ index: ConnectRoute });
    expect(screen.getByTestId('host-input')).toBeTruthy();
    expect(screen.getByTestId('token-input')).toBeTruthy();
    expect(screen.getByTestId('connect-button')).toBeTruthy();
  });

  it('shows an error for an invalid IP', async () => {
    renderRouter({ index: ConnectRoute });
    fireEvent.changeText(screen.getByTestId('host-input'), '999.999.999.999');
    fireEvent.changeText(screen.getByTestId('token-input'), MOCK_TOKEN);
    await act(async () => {
      fireEvent.press(screen.getByTestId('connect-button'));
    });
    expect(screen.getByText(/Invalid IP/i)).toBeTruthy();
    expect(useConnectionStore.getState().baseUrl).toBeNull();
  });

  it('rejects a bad token with a message and does not persist', async () => {
    renderRouter({ index: ConnectRoute });
    fireEvent.changeText(screen.getByTestId('host-input'), MOCK_HOST);
    fireEvent.changeText(screen.getByTestId('token-input'), 'wrong-token');
    await act(async () => {
      fireEvent.press(screen.getByTestId('connect-button'));
    });
    await waitFor(() => {
      expect(screen.getByText(/rejected/i)).toBeTruthy();
    });
    expect(useConnectionStore.getState().token).toBeNull();
  });

  it('connects and persists with a valid host + token', async () => {
    renderRouter({ index: ConnectRoute });
    fireEvent.changeText(screen.getByTestId('host-input'), MOCK_HOST);
    fireEvent.changeText(screen.getByTestId('token-input'), MOCK_TOKEN);
    await act(async () => {
      fireEvent.press(screen.getByTestId('connect-button'));
    });
    await waitFor(() => {
      expect(useConnectionStore.getState().baseUrl).toBe(MOCK_BASE_URL);
      expect(useConnectionStore.getState().token).toBe(MOCK_TOKEN);
    });
  });
});
