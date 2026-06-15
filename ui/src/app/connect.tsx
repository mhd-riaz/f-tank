import { useRouter } from 'expo-router';
import { useState } from 'react';
import { Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { normalizeBaseUrl, probeDevice } from '@/api/discovery';
import { AppButton } from '@/components/AppButton';
import { AppCard } from '@/components/AppCard';
import { AppTextField } from '@/components/AppTextField';
import { Screen } from '@/components/Screen';
import { useConnectionStore } from '@/store/connection';

/**
 * Manual connect screen (I1). The user enters the device IP (shown on the OLED)
 * and the pairing token issued during BLE provisioning. We probe `/healthz`,
 * validate the token via `/api/v1/info`, then persist the connection.
 *
 * When BLE onboarding (I2) lands it will populate these and route here; mDNS
 * auto-discovery (firmware follow-up) will pre-fill the host.
 */
export default function ConnectScreen() {
  const router = useRouter();
  const setConnection = useConnectionStore((s) => s.setConnection);

  const [host, setHost] = useState('');
  const [token, setToken] = useState('');
  const [hostError, setHostError] = useState<string | undefined>();
  const [busy, setBusy] = useState(false);
  const [message, setMessage] = useState<string | undefined>();

  const onConnect = async () => {
    setMessage(undefined);
    const normalized = normalizeBaseUrl(host);
    if (!normalized.ok || !normalized.baseUrl) {
      setHostError(normalized.error);
      return;
    }
    setHostError(undefined);
    if (token.trim().length === 0) {
      setMessage('Enter the device token from pairing.');
      return;
    }

    setBusy(true);
    const outcome = await probeDevice(normalized.baseUrl, token.trim());
    setBusy(false);

    switch (outcome.status) {
      case 'ok':
        await setConnection(normalized.baseUrl, token.trim());
        router.replace('/');
        break;
      case 'unauthorized':
        setMessage('That token was rejected. Re-check the token from pairing.');
        break;
      case 'unreachable':
        setMessage('No device responded. Check the IP and that you are on the same Wi‑Fi.');
        break;
      case 'not-ftank':
        setMessage('That address responded but is not an f-tank device.');
        break;
      default:
        setMessage(outcome.message);
    }
  };

  return (
    <Screen>
      <Text style={styles.title}>Connect to your tank</Text>
      <Text style={styles.subtitle}>
        Enter the IP address shown on the controller’s display and the pairing token.
      </Text>

      <AppCard>
        <AppTextField
          label="Device IP address"
          value={host}
          onChangeText={setHost}
          placeholder="192.168.1.42"
          keyboardType="url"
          errorText={hostError}
          testID="host-input"
        />
        <AppTextField
          label="Pairing token"
          value={token}
          onChangeText={setToken}
          placeholder="Token from setup"
          secureTextEntry
          testID="token-input"
        />
      </AppCard>

      {message ? (
        <View style={styles.messageBox}>
          <Text style={styles.message}>{message}</Text>
        </View>
      ) : null}

      <AppButton
        label="Connect"
        onPress={onConnect}
        loading={busy}
        testID="connect-button"
      />
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  title: {
    fontSize: theme.fontSize.xl,
    fontWeight: '700',
    color: theme.colors.text,
  },
  subtitle: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
  },
  messageBox: {
    backgroundColor: theme.colors.surface,
    borderRadius: theme.radius.md,
    borderWidth: 1,
    borderColor: theme.colors.danger,
    padding: theme.space.md,
  },
  message: {
    color: theme.colors.danger,
    fontSize: theme.fontSize.md,
  },
}));
