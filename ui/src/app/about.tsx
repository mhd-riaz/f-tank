import { Stack } from 'expo-router';
import Constants from 'expo-constants';
import { Text } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { APP_NAME, APP_TAGLINE } from '@/constants/app';
import { AppCard } from '@/components/AppCard';
import { InfoRow } from '@/components/InfoRow';
import { Screen } from '@/components/Screen';

/** About screen: app identity + version. */
export default function AboutScreen() {
  const appVersion = Constants.expoConfig?.version ?? '0.1.0';

  return (
    <Screen>
      <Stack.Screen options={{ headerShown: true, title: 'About' }} />

      <Text style={styles.title}>{APP_NAME}</Text>
      <Text style={styles.subtitle}>{APP_TAGLINE}</Text>

      <AppCard title="App">
        <InfoRow label="Version" value={appVersion} />
        <InfoRow label="Mode" value="Local (LAN)" />
      </AppCard>

      <Text style={styles.body}>
        f-tank drives your aquarium’s relay-controlled appliances on time-based schedules and
        monitors water temperature. This app talks directly to the controller over your local
        network — no account required.
      </Text>
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  title: {
    fontSize: theme.fontSize.xxl,
    fontWeight: '800',
    color: theme.colors.text,
  },
  subtitle: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
  },
  body: {
    fontSize: theme.fontSize.md,
    color: theme.colors.text,
    lineHeight: 22,
  },
}));
