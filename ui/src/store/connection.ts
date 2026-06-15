import * as SecureStore from 'expo-secure-store';
import { create } from 'zustand';

import { DeviceClient } from '@/api/client';

/**
 * Connection store: where the device lives on the LAN (`baseUrl`) and the
 * per-device bearer token. The token is sensitive, so it is persisted in the
 * platform secure enclave/keystore via expo-secure-store — never plain storage.
 *
 * The cloud phase will extend this with a remote base URL + account session;
 * the local-first build needs only base URL + token.
 */

const TOKEN_KEY = 'ftank.device.token';
const BASEURL_KEY = 'ftank.device.baseUrl';

export type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error';

interface ConnectionState {
  baseUrl: string | null;
  token: string | null;
  status: ConnectionStatus;
  hydrated: boolean;

  /** Load persisted connection (call once on app start). */
  hydrate: () => Promise<void>;
  /** Persist a verified connection. */
  setConnection: (baseUrl: string, token: string) => Promise<void>;
  setStatus: (status: ConnectionStatus) => void;
  /** Forget the device (logout / unpair locally). */
  clear: () => Promise<void>;
  /** Build an authenticated client for the current connection, or null. */
  getClient: () => DeviceClient | null;
}

export const useConnectionStore = create<ConnectionState>((set, get) => ({
  baseUrl: null,
  token: null,
  status: 'disconnected',
  hydrated: false,

  hydrate: async () => {
    const [token, baseUrl] = await Promise.all([
      SecureStore.getItemAsync(TOKEN_KEY),
      SecureStore.getItemAsync(BASEURL_KEY),
    ]);
    set({
      token: token ?? null,
      baseUrl: baseUrl ?? null,
      hydrated: true,
      status: token && baseUrl ? 'disconnected' : 'disconnected',
    });
  },

  setConnection: async (baseUrl, token) => {
    await Promise.all([
      SecureStore.setItemAsync(TOKEN_KEY, token),
      SecureStore.setItemAsync(BASEURL_KEY, baseUrl),
    ]);
    set({ baseUrl, token, status: 'connected' });
  },

  setStatus: (status) => set({ status }),

  clear: async () => {
    await Promise.all([
      SecureStore.deleteItemAsync(TOKEN_KEY),
      SecureStore.deleteItemAsync(BASEURL_KEY),
    ]);
    set({ baseUrl: null, token: null, status: 'disconnected' });
  },

  getClient: () => {
    const { baseUrl, token } = get();
    if (!baseUrl || !token) {
      return null;
    }
    return new DeviceClient({ baseUrl, token });
  },
}));
