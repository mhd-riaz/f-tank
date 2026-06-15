import { Stack } from 'expo-router';
import { Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { useDeviceConfig, useDeviceInfo } from '@/api/queries';
import {
  decodeAlerts,
  formatMinuteOfDay,
  formatTemperature,
  timeSourceLabel,
  wifiStateLabel,
} from '@/api/status';
import { AppCard } from '@/components/AppCard';
import { ChannelTile } from '@/components/ChannelTile';
import { InfoRow } from '@/components/InfoRow';
import { liveSourcePill, severityTone } from '@/components/presenters';
import { Screen } from '@/components/Screen';
import { StatusPill } from '@/components/StatusPill';
import { useLiveStatus } from '@/hooks/useLiveStatus';
import { useConnectionStore } from '@/store/connection';

/**
 * Live tank details (FR-33): the full real-time picture — temperature, time
 * source, Wi‑Fi state, device clock, every channel, active alerts, and firmware
 * info — all driven by the live status stream.
 */
export default function DetailsScreen() {
  const baseUrl = useConnectionStore((s) => s.baseUrl);
  const token = useConnectionStore((s) => s.token);

  const { status, source, lastUpdate } = useLiveStatus(baseUrl, token);
  const { data: config } = useDeviceConfig();
  const { data: info } = useDeviceInfo();

  const pill = liveSourcePill(source);
  const alerts = status ? decodeAlerts(status.alerts) : [];

  return (
    <Screen>
      <Stack.Screen options={{ headerShown: true, title: 'Live details' }} />

      <View style={styles.header}>
        <StatusPill label={pill.label} tone={pill.tone} />
        {lastUpdate ? (
          <Text style={styles.muted}>
            Updated {new Date(lastUpdate).toLocaleTimeString()}
          </Text>
        ) : null}
      </View>

      <AppCard title="Environment">
        <InfoRow label="Temperature" value={status ? formatTemperature(status) : '—'} />
        <InfoRow
          label="Device time"
          value={status ? formatMinuteOfDay(status.minuteOfDay) : '—'}
        />
        <InfoRow label="Time source" value={status ? timeSourceLabel(status.timeSource) : '—'} />
        <InfoRow label="Wi‑Fi" value={status ? wifiStateLabel(status.wifiState) : '—'} />
      </AppCard>

      <AppCard title="Alerts">
        {alerts.length === 0 ? (
          <Text style={styles.muted}>No active alerts.</Text>
        ) : (
          alerts.map((a) => (
            <View key={a.bit} style={styles.alertRow}>
              <StatusPill label={a.label} tone={severityTone(a.severity)} />
            </View>
          ))
        )}
      </AppCard>

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

      {info ? (
        <AppCard title="Device">
          <InfoRow label="Firmware" value={info.firmware} />
          <InfoRow label="Config version" value={String(info.configVersion)} />
          {info.channelCount !== undefined ? (
            <InfoRow label="Channels" value={String(info.channelCount)} />
          ) : null}
        </AppCard>
      ) : null}
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  channels: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: theme.space.md,
  },
  alertRow: {
    flexDirection: 'row',
  },
  muted: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
  },
}));
