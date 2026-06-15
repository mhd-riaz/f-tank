import { act, fireEvent, screen, waitFor } from '@testing-library/react-native';
import { renderRouter } from 'expo-router/testing-library';
import { QueryClientProvider, QueryClient } from '@tanstack/react-query';

import LogsScreen from '@/app/logs';
import { MOCK_BASE_URL, MOCK_TOKEN } from '@/mocks/handlers';
import { useConnectionStore } from '@/store/connection';

/**
 * Render + flow test for the on-demand event log screen. Proves it mounts,
 * lists decoded records from the mock device, toggles recording, and clears the
 * buffer.
 */
function makeWrapper() {
  const client = new QueryClient({ defaultOptions: { queries: { retry: false } } });
  return function Logs() {
    return (
      <QueryClientProvider client={client}>
        <LogsScreen />
      </QueryClientProvider>
    );
  };
}

describe('LogsScreen (render + flow)', () => {
  beforeEach(async () => {
    await useConnectionStore.getState().setConnection(MOCK_BASE_URL, MOCK_TOKEN);
  });
  afterEach(async () => {
    await useConnectionStore.getState().clear();
  });

  it('lists decoded log records', async () => {
    renderRouter({ logs: makeWrapper() }, { initialUrl: '/logs' });
    await waitFor(() => {
      // Mock device seeds a "Device started" (boot) record.
      expect(screen.getByText('Device started')).toBeTruthy();
    });
  });

  it('toggles recording on', async () => {
    renderRouter({ logs: makeWrapper() }, { initialUrl: '/logs' });
    await waitFor(() => expect(screen.getByTestId('recording-toggle')).toBeTruthy());

    await act(async () => {
      fireEvent(screen.getByTestId('recording-toggle'), 'valueChange', true);
    });

    await waitFor(() => {
      expect(screen.getByTestId('recording-toggle').props.value).toBe(true);
    });
  });

  it('clears the buffer', async () => {
    renderRouter({ logs: makeWrapper() }, { initialUrl: '/logs' });
    await waitFor(() => expect(screen.getByText('Device started')).toBeTruthy());

    await act(async () => {
      fireEvent.press(screen.getByTestId('clear-logs'));
    });

    await waitFor(() => {
      expect(screen.queryByText('Device started')).toBeNull();
    });
  });
});
