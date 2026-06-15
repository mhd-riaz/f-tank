import { Redirect, useRouter } from 'expo-router';
import { Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { useDeviceConfig } from '@/api/queries';
import { decodeAlerts, formatTemperature } from '@/api/status';
import { AppButton } from '@/components/AppButton';
import { AppCard } from '@/components/AppCard';
import { ChannelTile } from '@/components/ChannelTile';
import { liveSourcePill, severityTone } from '@/components/presenters';
import { Screen } from '@/components/Screen';
import { StatusPill } from '@/components/StatusPill';
import { useLiveStatus } from '@/hooks/useLiveStatus';
import { useConnectionStore } from '@/store/connection';

/**
 * Home dashboard (FR-33). Shows live temperature, channel states, and active
 * alerts streamed from the device WebSocket (with polling fallback). When no
 * device is paired it redirects to the connect flow.
 */
export default function DashboardScreen() {
  const router = useRouter();
  const hydrated = useConnectionStore((s) => s.hydrated);
  const baseUrl = useConnectionStore((s) => s.baseUrl);
  const token = useConnectionStore((s) => s.token);

  const { status, source } = useLiveStatus(baseUrl, token);
  const { data: config } = useDeviceConfig();

  if (!hydrated) {
    return (
      <Screen scroll={false}>
        <View style={styles.center}>
          <Text style={styles.muted}>Loading…</Text>
        </View>
      </Screen>
    );
  }

  if (!baseUrl || !token) {
    return <Redirect href="/connect" />;
  }

  const pill = liveSourcePill(source);
  const alerts = status ? decodeAlerts(status.alerts) : [];

  return (
    <Screen>
      <View style={styles.header}>
        <Text style={styles.title}>f-tank</Text>
        <StatusPill label={pill.label} tone={pill.tone} />
      </View>

      <AppCard title="Water temperature">
        <Text style={styles.temp}>{status ? formatTemperature(status) : '—'}</Text>
      </AppCard>

      {alerts.length > 0 ? (
        <AppCard title="Active alerts">
          {alerts.map((a) => (
            <View key={a.bit} style={styles.alertRow}>
              <StatusPill label={a.label} tone={severityTone(a.severity)} />
            </View>
          ))}
        </AppCard>
      ) : null}

      <AppCard title="Channels">
        {status ? (
          <View style={styles.channels}>
            {status.channels.map((energized, i) => (
              <ChannelTile
                key={i}
                index={i}
                name={config?.channels[i]?.name}
                energized={energized}
              />
            ))}
          </View>
        ) : (
          <Text style={styles.muted}>Waiting for live data…</Text>
        )}
      </AppCard>

      <View style={styles.actions}>
        <AppButton
          label="View live details"
          variant="outline"
          onPress={() => router.push('/details')}
        />
        <AppButton
          label="Schedules"
          variant="outline"
          onPress={() => router.push('/schedules')}
        />
        <AppButton label="More" variant="outline" onPress={() => router.push('/more')} />
      </View>
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  title: {
    fontSize: theme.fontSize.xxl,
    fontWeight: '700',
    color: theme.colors.text,
  },
  temp: {
    fontSize: 44,
    fontWeight: '800',
    color: theme.colors.text,
  },
  channels: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: theme.space.md,
  },
  alertRow: {
    flexDirection: 'row',
  },
  actions: {
    gap: theme.space.md,
  },
  center: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  muted: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
  },
}));
