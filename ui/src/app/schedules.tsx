import { Stack, useRouter } from 'expo-router';
import { ActivityIndicator, Pressable, Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { useDeviceConfig } from '@/api/queries';
import { describeWindow } from '@/api/schedule';
import { AppCard } from '@/components/AppCard';
import { Screen } from '@/components/Screen';
import { StatusPill } from '@/components/StatusPill';

/**
 * Schedules overview (FR-14): lists every channel with a summary of its
 * schedule; tapping a channel opens its editor. Channel count and names come
 * from the device config.
 */
export default function SchedulesScreen() {
  const router = useRouter();
  const { data: config, isLoading, isError, refetch } = useDeviceConfig();

  return (
    <Screen>
      <Stack.Screen options={{ headerShown: true, title: 'Schedules' }} />

      {isLoading ? (
        <View style={styles.center}>
          <ActivityIndicator size="large" />
        </View>
      ) : isError || !config ? (
        <AppCard>
          <Text style={styles.muted}>Couldn’t load the device config.</Text>
          <Pressable onPress={() => refetch()}>
            <Text style={styles.link}>Retry</Text>
          </Pressable>
        </AppCard>
      ) : (
        config.channels.map((channel, index) => {
          const windowCount = channel.schedule.windows.length;
          const summary =
            windowCount === 0
              ? 'No windows'
              : channel.schedule.windows.map(describeWindow).join(', ');
          return (
            <Pressable
              key={index}
              onPress={() => router.push(`/channel/${index}`)}
              testID={`channel-${index}`}
            >
              <AppCard>
                <View style={styles.headerRow}>
                  <Text style={styles.name}>{channel.name}</Text>
                  <StatusPill
                    label={channel.enabled ? 'Enabled' : 'Disabled'}
                    tone={channel.enabled ? 'success' : 'neutral'}
                  />
                </View>
                <Text style={styles.summary} numberOfLines={2}>
                  {channel.schedule.inverted ? 'OFF when ' : 'ON when '}
                  {summary}
                </Text>
              </AppCard>
            </Pressable>
          );
        })
      )}
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  center: {
    paddingVertical: theme.space.xxl,
    alignItems: 'center',
  },
  headerRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  name: {
    fontSize: theme.fontSize.lg,
    fontWeight: '700',
    color: theme.colors.text,
  },
  summary: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
  },
  muted: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
  },
  link: {
    fontSize: theme.fontSize.md,
    color: theme.colors.primary,
    fontWeight: '600',
  },
}));
