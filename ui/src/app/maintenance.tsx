import { Stack, useRouter } from 'expo-router';
import { useState } from 'react';
import { Alert, Text } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import type { ResetScope } from '@/api/client';
import { AppButton } from '@/components/AppButton';
import { AppCard } from '@/components/AppCard';
import { Screen } from '@/components/Screen';
import { useConnectionStore } from '@/store/connection';

/**
 * Maintenance: factory reset and forget-WiFi (FR-10). Both are confirm-gated
 * and destructive; either one wipes the device token, so afterwards the app
 * clears its stored connection and returns to the connect/onboarding flow.
 */
export default function MaintenanceScreen() {
  const router = useRouter();
  const getClient = useConnectionStore((s) => s.getClient);
  const clear = useConnectionStore((s) => s.clear);
  const [busy, setBusy] = useState<ResetScope | null>(null);
  const [error, setError] = useState<string | undefined>();

  const doReset = async (scope: ResetScope) => {
    setError(undefined);
    setBusy(scope);
    try {
      const client = getClient();
      if (!client) {
        throw new Error('not connected');
      }
      await client.reset(scope);
      // The device drops its token on reset; forget the local connection too.
      await clear();
      router.replace('/connect');
    } catch (e) {
      setError((e as Error).message ?? 'Reset failed.');
    } finally {
      setBusy(null);
    }
  };

  const confirm = (scope: ResetScope, title: string, message: string) => {
    Alert.alert(title, message, [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Confirm', style: 'destructive', onPress: () => doReset(scope) },
    ]);
  };

  return (
    <Screen>
      <Stack.Screen options={{ headerShown: true, title: 'Maintenance' }} />

      <AppCard title="Forget Wi‑Fi">
        <Text style={styles.body}>
          Clears the saved Wi‑Fi network and pairing token so you can move the controller to a
          different network. Schedules and settings are kept.
        </Text>
        <AppButton
          label="Forget Wi‑Fi"
          variant="outline"
          loading={busy === 'wifi'}
          onPress={() =>
            confirm(
              'wifi',
              'Forget Wi‑Fi?',
              'The controller will need to be set up again to reconnect.',
            )
          }
          testID="forget-wifi"
        />
      </AppCard>

      <AppCard title="Factory reset">
        <Text style={styles.body}>
          Erases everything — Wi‑Fi, pairing token, schedules, thresholds, and logs — and restores
          factory defaults. This cannot be undone.
        </Text>
        <AppButton
          label="Factory reset"
          variant="outline"
          loading={busy === 'factory'}
          onPress={() =>
            confirm(
              'factory',
              'Factory reset?',
              'This erases all settings and unpairs the device. This cannot be undone.',
            )
          }
          testID="factory-reset"
        />
      </AppCard>

      {error ? <Text style={styles.error}>{error}</Text> : null}
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  body: {
    fontSize: theme.fontSize.md,
    color: theme.colors.text,
    lineHeight: 22,
  },
  error: {
    fontSize: theme.fontSize.md,
    color: theme.colors.danger,
  },
}));
