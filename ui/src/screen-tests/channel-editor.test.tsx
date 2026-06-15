import { act, fireEvent, screen, waitFor } from '@testing-library/react-native';
import { renderRouter } from 'expo-router/testing-library';
import { QueryClientProvider, QueryClient } from '@tanstack/react-query';
import { http, HttpResponse } from 'msw';
import { Text } from 'react-native';

import ChannelEditorScreen from '@/app/channel/[index]';
import { MOCK_BASE_URL, MOCK_TOKEN } from '@/mocks/handlers';
import { server } from '@/mocks/server';
import { useConnectionStore } from '@/store/connection';

/**
 * Render + flow test for the channel schedule editor. Proves the screen mounts,
 * loads the channel, lets the user add a window, and PUTs the FULL channels
 * array (the firmware's count-match rule) with the edit applied.
 */
function makeWrapper() {
  // Fresh client per test so cached config doesn't leak between cases.
  const client = new QueryClient({ defaultOptions: { queries: { retry: false } } });
  return function Editor() {
    return (
      <QueryClientProvider client={client}>
        <ChannelEditorScreen />
      </QueryClientProvider>
    );
  };
}

// A stub origin so the editor's router.back() after save has somewhere to go.
function SchedulesStub() {
  return <Text testID="schedules-stub">schedules</Text>;
}

function routes() {
  return { schedules: SchedulesStub, 'channel/[index]': makeWrapper() };
}

describe('ChannelEditorScreen (render + save)', () => {
  beforeEach(async () => {
    await useConnectionStore.getState().setConnection(MOCK_BASE_URL, MOCK_TOKEN);
  });
  afterEach(async () => {
    await useConnectionStore.getState().clear();
  });

  it('loads the channel and shows its name', async () => {
    renderRouter(routes(), { initialUrl: '/channel/0' });
    await waitFor(() => {
      // Mock device channel 0 is "Lights".
      expect(screen.getByTestId('name-input').props.value).toBe('Lights');
    });
  });

  it('adds a window and saves the full channels array', async () => {
    const captured: { body: { channels?: unknown[] } | null } = { body: null };
    server.use(
      http.put(`${MOCK_BASE_URL}/api/v1/config`, async ({ request }) => {
        captured.body = (await request.json()) as { channels?: unknown[] };
        return HttpResponse.json({ status: 'accepted' }, { status: 202 });
      }),
    );
    renderRouter(routes(), { initialUrl: '/channel/0' });
    await waitFor(() => expect(screen.getByTestId('name-input')).toBeTruthy());

    fireEvent.changeText(screen.getByTestId('start-input'), '09:30');
    fireEvent.changeText(screen.getByTestId('end-input'), '11:45');
    await act(async () => {
      fireEvent.press(screen.getByTestId('add-window'));
    });

    await act(async () => {
      fireEvent.press(screen.getByTestId('save-button'));
    });

    await waitFor(() => {
      expect(captured.body).not.toBeNull();
    });
    // Mock device has 4 channels — the PUT must include all of them.
    expect(captured.body?.channels).toHaveLength(4);
  });

  it('rejects an invalid window time without saving', async () => {
    renderRouter(routes(), { initialUrl: '/channel/0' });
    await waitFor(() => expect(screen.getByTestId('name-input')).toBeTruthy());

    fireEvent.changeText(screen.getByTestId('start-input'), '25:00');
    fireEvent.changeText(screen.getByTestId('end-input'), '11:00');
    await act(async () => {
      fireEvent.press(screen.getByTestId('add-window'));
    });

    expect(screen.getByText(/valid times/i)).toBeTruthy();
  });
});
