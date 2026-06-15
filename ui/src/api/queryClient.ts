import { QueryClient } from '@tanstack/react-query';

/**
 * Shared TanStack Query client. Device data is LAN-local and cheap to refetch;
 * defaults favor freshness without hammering the ESP32.
 */
export const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      staleTime: 5_000,
      retry: 1,
      refetchOnWindowFocus: false,
    },
  },
});
