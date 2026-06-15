import { useQuery } from '@tanstack/react-query';

import { useConnectionStore } from '@/store/connection';

/**
 * TanStack Query hooks for the device's REST resources. Keyed by base URL so a
 * device change refetches cleanly. Live status uses a dedicated WebSocket hook
 * (`useLiveStatus`) rather than polling here.
 */

export function useDeviceInfo() {
  const baseUrl = useConnectionStore((s) => s.baseUrl);
  const getClient = useConnectionStore((s) => s.getClient);
  return useQuery({
    queryKey: ['device', baseUrl, 'info'],
    enabled: Boolean(baseUrl),
    queryFn: () => {
      const client = getClient();
      if (!client) {
        throw new Error('not connected');
      }
      return client.getInfo();
    },
  });
}

export function useDeviceConfig() {
  const baseUrl = useConnectionStore((s) => s.baseUrl);
  const getClient = useConnectionStore((s) => s.getClient);
  return useQuery({
    queryKey: ['device', baseUrl, 'config'],
    enabled: Boolean(baseUrl),
    queryFn: () => {
      const client = getClient();
      if (!client) {
        throw new Error('not connected');
      }
      return client.getConfig();
    },
  });
}
