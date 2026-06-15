import { Stack, useLocalSearchParams, useRouter } from 'expo-router';
import { useMemo, useState } from 'react';
import { ActivityIndicator, Alert, Pressable, Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { useDeviceConfig } from '@/api/queries';
import { useUpdateConfig } from '@/api/mutations';
import {
  addWindow,
  buildConfigUpdate,
  clearSchedule,
  describeWindow,
  parseHhMm,
  removeWindow,
  setInverted,
} from '@/api/schedule';
import { isValidChannelName } from '@/api/validation';
import type { ChannelConfig } from '@/api/schemas';
import { AppButton } from '@/components/AppButton';
import { AppCard } from '@/components/AppCard';
import { AppTextField } from '@/components/AppTextField';
import { Screen } from '@/components/Screen';
import { ToggleRow } from '@/components/ToggleRow';

/**
 * Channel schedule editor (FR-14): edit name, enable flag, the inverted
 * (OFF-window) flag, and the on/off windows. Read-only provisioned fields
 * (polarity, heater-cut target) are shown but not editable (FR-12/24). Saving
 * sends the FULL channels array so the firmware's count-match rule is met.
 */
export default function ChannelEditorScreen() {
  const router = useRouter();
  const params = useLocalSearchParams<{ index: string }>();
  const channelIndex = Number(params.index);

  const { data: config, isLoading } = useDeviceConfig();
  const updateConfig = useUpdateConfig();

  const original = config?.channels[channelIndex];

  // Local draft seeded from the loaded channel.
  const [draft, setDraft] = useState<ChannelConfig | null>(null);
  const [startText, setStartText] = useState('');
  const [endText, setEndText] = useState('');
  const [error, setError] = useState<string | undefined>();

  // Initialize the draft once config loads.
  const channel = useMemo<ChannelConfig | null>(() => {
    if (draft) {
      return draft;
    }
    return original ?? null;
  }, [draft, original]);

  if (isLoading || !config) {
    return (
      <Screen scroll={false}>
        <Stack.Screen options={{ headerShown: true, title: 'Channel' }} />
        <View style={styles.center}>
          <ActivityIndicator size="large" />
        </View>
      </Screen>
    );
  }

  if (!original || !channel || Number.isNaN(channelIndex)) {
    return (
      <Screen>
        <Stack.Screen options={{ headerShown: true, title: 'Channel' }} />
        <Text style={styles.muted}>Channel not found.</Text>
      </Screen>
    );
  }

  const update = (next: ChannelConfig) => {
    setDraft(next);
    setError(undefined);
  };

  const onAddWindow = () => {
    const start = parseHhMm(startText);
    const end = parseHhMm(endText);
    if (start === null || end === null) {
      setError('Enter valid times as HH:MM (e.g. 08:00).');
      return;
    }
    const result = addWindow(channel.schedule, { start, end });
    if (!result.ok) {
      setError(result.error);
      return;
    }
    update({ ...channel, schedule: result.schedule });
    setStartText('');
    setEndText('');
  };

  const onRemoveWindow = (i: number) => {
    update({ ...channel, schedule: removeWindow(channel.schedule, i) });
  };

  const onSave = async () => {
    if (!isValidChannelName(channel.name)) {
      setError('Name must be 1–23 printable characters.');
      return;
    }
    const patch = buildConfigUpdate(config, channelIndex, channel);
    try {
      await updateConfig.mutateAsync(patch);
      if (router.canGoBack()) {
        router.back();
      } else {
        router.replace('/schedules');
      }
    } catch (e) {
      setError((e as Error).message ?? 'Failed to save.');
    }
  };

  const onResetSchedule = () => {
    Alert.alert('Reset schedule?', 'This clears all windows for this channel.', [
      { text: 'Cancel', style: 'cancel' },
      {
        text: 'Reset',
        style: 'destructive',
        onPress: () => update({ ...channel, schedule: clearSchedule(channel.schedule) }),
      },
    ]);
  };

  return (
    <Screen>
      <Stack.Screen options={{ headerShown: true, title: channel.name || 'Channel' }} />

      <AppCard title="Channel">
        <AppTextField
          label="Name"
          value={channel.name}
          onChangeText={(name) => update({ ...channel, name })}
          autoCapitalize="words"
          testID="name-input"
        />
        <ToggleRow
          label="Enabled"
          description="Run this channel’s schedule"
          value={channel.enabled}
          onValueChange={(enabled) => update({ ...channel, enabled })}
          testID="enabled-toggle"
        />
      </AppCard>

      <AppCard title="Schedule">
        <ToggleRow
          label="Invert (OFF windows)"
          description={
            channel.schedule.inverted
              ? 'Windows below are OFF periods; channel is ON otherwise.'
              : 'Windows below are ON periods; channel is OFF otherwise.'
          }
          value={channel.schedule.inverted}
          onValueChange={(inv) => update({ ...channel, schedule: setInverted(channel.schedule, inv) })}
          testID="invert-toggle"
        />

        {channel.schedule.windows.length === 0 ? (
          <Text style={styles.muted}>No windows yet.</Text>
        ) : (
          channel.schedule.windows.map((win, i) => (
            <View key={i} style={styles.windowRow}>
              <Text style={styles.windowText}>{describeWindow(win)}</Text>
              <Pressable onPress={() => onRemoveWindow(i)} testID={`remove-window-${i}`}>
                <Text style={styles.remove}>Remove</Text>
              </Pressable>
            </View>
          ))
        )}

        <View style={styles.addRow}>
          <View style={styles.addField}>
            <AppTextField
              label="Start"
              value={startText}
              onChangeText={setStartText}
              placeholder="08:00"
              testID="start-input"
            />
          </View>
          <View style={styles.addField}>
            <AppTextField
              label="End"
              value={endText}
              onChangeText={setEndText}
              placeholder="18:00"
              testID="end-input"
            />
          </View>
        </View>
        <AppButton label="Add window" variant="outline" onPress={onAddWindow} testID="add-window" />
      </AppCard>

      <AppCard title="Provisioned (read-only)">
        <Text style={styles.muted}>
          Polarity: {channel.polarity === 0 ? 'Active-high' : 'Active-low'}
        </Text>
        <Text style={styles.muted}>
          Heater-cut target: {channel.cutOnOverTemp ? 'Yes' : 'No'}
        </Text>
      </AppCard>

      {error ? <Text style={styles.error}>{error}</Text> : null}

      <AppButton
        label="Save changes"
        onPress={onSave}
        loading={updateConfig.isPending}
        testID="save-button"
      />
      <AppButton label="Reset schedule" variant="outline" onPress={onResetSchedule} />
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  center: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  windowRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingVertical: theme.space.sm,
    borderBottomWidth: 1,
    borderBottomColor: theme.colors.border,
  },
  windowText: {
    fontSize: theme.fontSize.md,
    color: theme.colors.text,
  },
  remove: {
    fontSize: theme.fontSize.md,
    color: theme.colors.danger,
    fontWeight: '600',
  },
  addRow: {
    flexDirection: 'row',
    gap: theme.space.md,
  },
  addField: {
    flex: 1,
  },
  muted: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
  },
  error: {
    fontSize: theme.fontSize.md,
    color: theme.colors.danger,
  },
}));
