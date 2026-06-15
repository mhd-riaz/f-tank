import { Stack, useRouter } from 'expo-router';
import { useMemo, useState } from 'react';
import { ActivityIndicator, Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { useDeviceConfig } from '@/api/queries';
import { useUpdateConfig } from '@/api/mutations';
import { isValidTz, validateThresholds } from '@/api/validation';
import type { Thresholds } from '@/api/schemas';
import { AppButton } from '@/components/AppButton';
import { AppCard } from '@/components/AppCard';
import { AppTextField } from '@/components/AppTextField';
import { Screen } from '@/components/Screen';

/**
 * Device settings (FR-22, FR-7a): temperature alert thresholds and the POSIX
 * timezone string. Thresholds and tz are independent of channels, so we PUT
 * only those fields (the firmware overlays known fields).
 */
export default function SettingsScreen() {
  const router = useRouter();
  const { data: config, isLoading } = useDeviceConfig();
  const updateConfig = useUpdateConfig();

  const [low, setLow] = useState<string | null>(null);
  const [high, setHigh] = useState<string | null>(null);
  const [hyst, setHyst] = useState<string | null>(null);
  const [tz, setTz] = useState<string | null>(null);
  const [error, setError] = useState<string | undefined>();

  // Seed text fields from the loaded config (first render after load).
  const seeded = useMemo(() => {
    if (!config) {
      return null;
    }
    return {
      low: low ?? String(config.thresholds.lowC),
      high: high ?? String(config.thresholds.highC),
      hyst: hyst ?? String(config.thresholds.hysteresisC),
      tz: tz ?? config.tz,
    };
  }, [config, low, high, hyst, tz]);

  if (isLoading || !config || !seeded) {
    return (
      <Screen scroll={false}>
        <Stack.Screen options={{ headerShown: true, title: 'Settings' }} />
        <View style={styles.center}>
          <ActivityIndicator size="large" />
        </View>
      </Screen>
    );
  }

  const onSave = async () => {
    setError(undefined);
    const next: Thresholds = {
      lowC: Number(seeded.low),
      highC: Number(seeded.high),
      hysteresisC: Number(seeded.hyst),
    };
    if ([next.lowC, next.highC, next.hysteresisC].some((n) => Number.isNaN(n))) {
      setError('Temperatures must be numbers.');
      return;
    }
    const tCheck = validateThresholds(next);
    if (!tCheck.ok) {
      setError(tCheck.error);
      return;
    }
    if (!isValidTz(seeded.tz)) {
      setError('Timezone must be 1–47 printable characters (e.g. IST-5:30).');
      return;
    }
    try {
      await updateConfig.mutateAsync({ thresholds: next, tz: seeded.tz });
      if (router.canGoBack()) {
        router.back();
      } else {
        router.replace('/more');
      }
    } catch (e) {
      setError((e as Error).message ?? 'Failed to save.');
    }
  };

  return (
    <Screen>
      <Stack.Screen options={{ headerShown: true, title: 'Settings' }} />

      <AppCard title="Temperature alerts (°C)">
        <AppTextField
          label="Low threshold"
          value={seeded.low}
          onChangeText={setLow}
          keyboardType="numeric"
          testID="low-input"
        />
        <AppTextField
          label="High threshold"
          value={seeded.high}
          onChangeText={setHigh}
          keyboardType="numeric"
          testID="high-input"
        />
        <AppTextField
          label="Hysteresis"
          value={seeded.hyst}
          onChangeText={setHyst}
          keyboardType="numeric"
          helperText="Re-entry margin to avoid flapping (0–5)."
          testID="hyst-input"
        />
      </AppCard>

      <AppCard title="Timezone">
        <AppTextField
          label="POSIX TZ string"
          value={seeded.tz}
          onChangeText={setTz}
          helperText="e.g. UTC0, IST-5:30, GMT0BST,M3.5.0/1,M10.5.0"
          testID="tz-input"
        />
      </AppCard>

      {error ? <Text style={styles.error}>{error}</Text> : null}

      <AppButton
        label="Save settings"
        onPress={onSave}
        loading={updateConfig.isPending}
        testID="save-settings"
      />
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  center: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  error: {
    fontSize: theme.fontSize.md,
    color: theme.colors.danger,
  },
}));
