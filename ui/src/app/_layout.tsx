// Unistyles config has a side effect (StyleSheet.configure) and MUST be imported
// before any styled component renders — keep this as the very first import.
import '@/theme/unistyles';

import { QueryClientProvider } from '@tanstack/react-query';
import { Stack } from 'expo-router';
import { StatusBar } from 'expo-status-bar';
import { useEffect } from 'react';
import { GestureHandlerRootView } from 'react-native-gesture-handler';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import { queryClient } from '@/api/queryClient';
import { useConnectionStore } from '@/store/connection';

/**
 * Root layout: wires the server-state cache (TanStack Query) and
 * gesture/safe-area providers, then hydrates the persisted device connection
 * before rendering routes. Theming is provided by Unistyles (imported above),
 * which needs no React provider.
 */
export default function RootLayout() {
  const hydrate = useConnectionStore((s) => s.hydrate);

  useEffect(() => {
    void hydrate();
  }, [hydrate]);

  return (
    <GestureHandlerRootView style={{ flex: 1 }}>
      <QueryClientProvider client={queryClient}>
        <SafeAreaProvider>
          <StatusBar style="auto" />
          <Stack screenOptions={{ headerShown: false }} />
        </SafeAreaProvider>
      </QueryClientProvider>
    </GestureHandlerRootView>
  );
}
