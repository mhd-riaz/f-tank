// Unistyles relies on Nitro native modules that don't exist under Jest. Its
// official mock stubs them out and MUST load before the theme config (which
// calls StyleSheet.configure). Both use require so the mock registers first
// (ES imports would hoist the theme above the mock) and so TypeScript doesn't
// follow into the mock's untyped source.
/* eslint-disable @typescript-eslint/no-require-imports */
require('react-native-unistyles/mocks');
require('@/theme/unistyles');
/* eslint-enable @typescript-eslint/no-require-imports */

import { resetMockDevice } from '@/mocks/handlers';
import { server } from '@/mocks/server';

/**
 * Global Jest setup: start the MSW mock device, reset state between tests, and
 * provide an in-memory expo-secure-store so the connection store is testable
 * without the native keystore.
 */

// In-memory SecureStore stand-in.
jest.mock('expo-secure-store', () => {
  const mem = new Map<string, string>();
  return {
    getItemAsync: jest.fn(async (k: string) => mem.get(k) ?? null),
    setItemAsync: jest.fn(async (k: string, v: string) => {
      mem.set(k, v);
    }),
    deleteItemAsync: jest.fn(async (k: string) => {
      mem.delete(k);
    }),
  };
});

beforeAll(() => server.listen({ onUnhandledRequest: 'error' }));
afterEach(() => {
  server.resetHandlers();
  resetMockDevice();
});
afterAll(() => server.close());
