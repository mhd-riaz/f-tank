import { useMutation, useQueryClient } from '@tanstack/react-query';

import type { DeviceConfig } from '@/api/schemas';
import { useConnectionStore } from '@/store/connection';

/**
 * Mutation to PUT a partial device config and refresh the cached config on
 * success. Callers pass only the fields they intend to change (the firmware
 * overlays known fields); for channel edits, pass the FULL channels array via
 * `buildConfigUpdate`.
 */
export function useUpdateConfig() {
  const baseUrl = useConnectionStore((s) => s.baseUrl);
  const getClient = useConnectionStore((s) => s.getClient);
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: async (patch: Partial<DeviceConfig>) => {
      const client = getClient();
      if (!client) {
        throw new Error('not connected');
      }
      await client.putConfig(patch);
    },
    onSuccess: () => {
      void queryClient.invalidateQueries({ queryKey: ['device', baseUrl, 'config'] });
      void queryClient.invalidateQueries({ queryKey: ['device', baseUrl, 'info'] });
    },
  });
}
