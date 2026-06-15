import { Stack } from 'expo-router';
import { useQuery } from '@tanstack/react-query';
import { useState } from 'react';
import { ActivityIndicator, FlatList, Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { useDeviceConfig } from '@/api/queries';
import { decodeLog } from '@/api/logs';
import { AppButton } from '@/components/AppButton';
import { AppCard } from '@/components/AppCard';
import { Screen } from '@/components/Screen';
import { StatusPill } from '@/components/StatusPill';
import { ToggleRow } from '@/components/ToggleRow';
import { useConnectionStore } from '@/store/connection';

/**
 * On-demand event log (FR-29). Logs are produced only while recording is
 * enabled, so this screen exposes the recording toggle, a clear action, and a
 * refreshable list of decoded records.
 */
export default function LogsScreen() {
  const baseUrl = useConnectionStore((s) => s.baseUrl);
  const getClient = useConnectionStore((s) => s.getClient);
  const { data: config } = useDeviceConfig();
  const [working, setWorking] = useState(false);

  const logsQuery = useQuery({
    queryKey: ['device', baseUrl, 'logs'],
    enabled: Boolean(baseUrl),
    queryFn: () => {
      const client = getClient();
      if (!client) {
        throw new Error('not connected');
      }
      return client.getLogs();
    },
  });

  const channelName = (i: number) => config?.channels[i]?.name ?? `Channel ${i + 1}`;

  const setRecording = async (enabled: boolean) => {
    const client = getClient();
    if (!client) return;
    setWorking(true);
    try {
      await client.setRecording({ enabled });
      await logsQuery.refetch();
    } finally {
      setWorking(false);
    }
  };

  const clearLogs = async () => {
    const client = getClient();
    if (!client) return;
    setWorking(true);
    try {
      await client.setRecording({ clear: true });
      await logsQuery.refetch();
    } finally {
      setWorking(false);
    }
  };

  const data = logsQuery.data;

  return (
    <Screen scroll={false}>
      <Stack.Screen options={{ headerShown: true, title: 'Event log' }} />

      <View style={styles.controls}>
        <AppCard title="Recording">
          <ToggleRow
            label="Record events"
            description="On-demand only — events are captured while this is on."
            value={data?.recording ?? false}
            onValueChange={setRecording}
            disabled={working || logsQuery.isLoading}
            testID="recording-toggle"
          />
          <View style={styles.row}>
            {data ? (
              <StatusPill
                label={`${data.records.length} buffered · ${data.persisted} saved`}
                tone="neutral"
              />
            ) : null}
          </View>
          <View style={styles.actions}>
            <AppButton
              label="Refresh"
              variant="outline"
              onPress={() => logsQuery.refetch()}
              testID="refresh-logs"
            />
            <AppButton
              label="Clear"
              variant="outline"
              onPress={clearLogs}
              testID="clear-logs"
            />
          </View>
        </AppCard>
      </View>

      {logsQuery.isLoading ? (
        <View style={styles.center}>
          <ActivityIndicator size="large" />
        </View>
      ) : !data || data.records.length === 0 ? (
        <Text style={styles.muted}>
          No events buffered. Turn on recording to capture device events.
        </Text>
      ) : (
        <FlatList
          data={data.records}
          keyExtractor={(_, i) => String(i)}
          contentContainerStyle={styles.list}
          renderItem={({ item }) => {
            const d = decodeLog(item, channelName);
            return (
              <View style={styles.logRow}>
                <View style={styles.logText}>
                  <Text style={styles.logTitle}>{d.title}</Text>
                  {d.detail ? <Text style={styles.logDetail}>{d.detail}</Text> : null}
                </View>
                <Text style={styles.logTime}>{d.time}</Text>
              </View>
            );
          }}
        />
      )}
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  controls: {
    gap: theme.space.md,
  },
  row: {
    flexDirection: 'row',
  },
  actions: {
    flexDirection: 'row',
    gap: theme.space.md,
  },
  center: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  list: {
    gap: theme.space.sm,
    paddingTop: theme.space.md,
  },
  logRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    gap: theme.space.md,
    paddingVertical: theme.space.sm,
    borderBottomWidth: 1,
    borderBottomColor: theme.colors.border,
  },
  logText: {
    flex: 1,
  },
  logTitle: {
    fontSize: theme.fontSize.md,
    color: theme.colors.text,
    fontWeight: '600',
  },
  logDetail: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.textMuted,
  },
  logTime: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.textMuted,
  },
  muted: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
    paddingTop: theme.space.lg,
  },
}));
