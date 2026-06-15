import { Stack, useRouter } from 'expo-router';
import * as WebBrowser from 'expo-web-browser';
import { Alert } from 'react-native';

import { SUPPORT_FORM_URL } from '@/constants/app';
import { AppButton } from '@/components/AppButton';
import { AppCard } from '@/components/AppCard';
import { Screen } from '@/components/Screen';
import { useConnectionStore } from '@/store/connection';

/**
 * "More" hub: secondary navigation for the local-first build — help/guides,
 * about, support (external Google Form), and disconnecting the device. Device
 * management (schedules, thresholds, reset, OTA) is added in I3/I4.
 */
export default function MoreScreen() {
  const router = useRouter();
  const clear = useConnectionStore((s) => s.clear);
  const baseUrl = useConnectionStore((s) => s.baseUrl);

  const openSupport = async () => {
    await WebBrowser.openBrowserAsync(SUPPORT_FORM_URL);
  };

  const onDisconnect = () => {
    Alert.alert('Disconnect device?', 'You can reconnect with the IP and token any time.', [
      { text: 'Cancel', style: 'cancel' },
      {
        text: 'Disconnect',
        style: 'destructive',
        onPress: async () => {
          await clear();
          router.replace('/connect');
        },
      },
    ]);
  };

  return (
    <Screen>
      <Stack.Screen options={{ headerShown: true, title: 'More' }} />

      {baseUrl ? (
        <AppCard title="Device">
          <AppButton
            label="Schedules"
            variant="outline"
            onPress={() => router.push('/schedules')}
          />
          <AppButton label="Settings" variant="outline" onPress={() => router.push('/settings')} />
          <AppButton label="Event log" variant="outline" onPress={() => router.push('/logs')} />
          <AppButton
            label="Firmware update"
            variant="outline"
            onPress={() => router.push('/ota')}
          />
          <AppButton
            label="Maintenance"
            variant="outline"
            onPress={() => router.push('/maintenance')}
          />
        </AppCard>
      ) : null}

      <AppCard title="Help & info">
        <AppButton label="User guides" variant="outline" onPress={() => router.push('/help')} />
        <AppButton label="Support / contact" variant="outline" onPress={openSupport} />
        <AppButton label="About" variant="outline" onPress={() => router.push('/about')} />
      </AppCard>

      {baseUrl ? (
        <AppCard title="Connection">
          <AppButton label="Disconnect this device" variant="outline" onPress={onDisconnect} />
        </AppCard>
      ) : null}
    </Screen>
  );
}
