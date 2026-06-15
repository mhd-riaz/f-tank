import { act, fireEvent, screen, waitFor } from '@testing-library/react-native';
import { renderRouter } from 'expo-router/testing-library';
import { Alert, Text } from 'react-native';
import { http, HttpResponse } from 'msw';

import MaintenanceScreen from '@/app/maintenance';
import { MOCK_BASE_URL, MOCK_TOKEN } from '@/mocks/handlers';
import { server } from '@/mocks/server';
import { useConnectionStore } from '@/store/connection';

/**
 * Render + flow test for maintenance (factory reset / forget-WiFi). Auto-confirms
 * the destructive Alert, verifies the correct scope is sent, and that the local
 * connection is cleared afterwards (the device drops its token on reset).
 */
function ConnectStub() {
  return <Text testID="connect-stub">connect</Text>;
}

function routes() {
  return { maintenance: MaintenanceScreen, connect: ConnectStub };
}

describe('MaintenanceScreen (reset flow)', () => {
  beforeEach(async () => {
    await useConnectionStore.getState().setConnection(MOCK_BASE_URL, MOCK_TOKEN);
  });
  afterEach(async () => {
    jest.restoreAllMocks();
    await useConnectionStore.getState().clear();
  });

  it('sends scope=factory and clears the connection on confirm', async () => {
    // Auto-press the destructive "Confirm" button in the Alert.
    jest.spyOn(Alert, 'alert').mockImplementation((_t, _m, buttons) => {
      const confirm = buttons?.find((b) => b.style === 'destructive');
      confirm?.onPress?.();
    });

    let sentScope: string | undefined;
    server.use(
      http.post(`${MOCK_BASE_URL}/api/v1/reset`, async ({ request }) => {
        const body = (await request.json()) as { scope?: string; confirm?: boolean };
        sentScope = body.scope;
        return HttpResponse.json({ status: 'resetting' }, { status: 202 });
      }),
    );

    renderRouter(routes(), { initialUrl: '/maintenance' });
    await waitFor(() => expect(screen.getByTestId('factory-reset')).toBeTruthy());

    await act(async () => {
      fireEvent.press(screen.getByTestId('factory-reset'));
    });

    await waitFor(() => {
      expect(sentScope).toBe('factory');
      expect(useConnectionStore.getState().token).toBeNull();
    });
  });

  it('sends scope=wifi for forget Wi‑Fi', async () => {
    jest.spyOn(Alert, 'alert').mockImplementation((_t, _m, buttons) => {
      buttons?.find((b) => b.style === 'destructive')?.onPress?.();
    });

    let sentScope: string | undefined;
    server.use(
      http.post(`${MOCK_BASE_URL}/api/v1/reset`, async ({ request }) => {
        const body = (await request.json()) as { scope?: string };
        sentScope = body.scope;
        return HttpResponse.json({ status: 'resetting' }, { status: 202 });
      }),
    );

    renderRouter(routes(), { initialUrl: '/maintenance' });
    await waitFor(() => expect(screen.getByTestId('forget-wifi')).toBeTruthy());

    await act(async () => {
      fireEvent.press(screen.getByTestId('forget-wifi'));
    });

    await waitFor(() => {
      expect(sentScope).toBe('wifi');
    });
  });
});
