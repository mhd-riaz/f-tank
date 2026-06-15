import { Stack } from 'expo-router';
import { Text } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { AppCard } from '@/components/AppCard';
import { Screen } from '@/components/Screen';

/**
 * User guides / help (stub). Content is intentionally placeholder for now — the
 * product owner will supply the real guides; structure is in place to drop them
 * in as cards or link out to hosted docs later.
 */
const TOPICS: { title: string; body: string }[] = [
  {
    title: 'Connecting to your tank',
    body: 'Find the IP address on the controller’s display, then enter it with your pairing token on the Connect screen.',
  },
  {
    title: 'Reading the dashboard',
    body: 'The dashboard shows live water temperature, each channel’s ON/OFF state, and any active alerts, updating in real time.',
  },
  {
    title: 'Schedules (coming soon)',
    body: 'You’ll be able to set daily on/off windows per channel. This arrives in an upcoming update.',
  },
];

export default function HelpScreen() {
  return (
    <Screen>
      <Stack.Screen options={{ headerShown: true, title: 'User guides' }} />
      <Text style={styles.intro}>Quick help for getting the most out of f-tank.</Text>
      {TOPICS.map((t) => (
        <AppCard key={t.title} title={t.title}>
          <Text style={styles.body}>{t.body}</Text>
        </AppCard>
      ))}
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  intro: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
  },
  body: {
    fontSize: theme.fontSize.md,
    color: theme.colors.text,
    lineHeight: 22,
  },
}));
