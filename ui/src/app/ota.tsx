import { Stack, useRouter } from 'expo-router';
import * as DocumentPicker from 'expo-document-picker';
import { useState } from 'react';
import { Alert, Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

import { validateOtaMetadata } from '@/api/ota';
import { AppButton } from '@/components/AppButton';
import { AppCard } from '@/components/AppCard';
import { AppTextField } from '@/components/AppTextField';
import { Screen } from '@/components/Screen';
import { useConnectionStore } from '@/store/connection';

/**
 * Local signed OTA upload (FR-38/39). The user selects a firmware image and
 * supplies the version, SHA-256, and signature from the signed release
 * artifact. The device verifies integrity, signature, and anti-rollback itself;
 * this screen only relays the bytes and headers.
 */
export default function OtaScreen() {
  const router = useRouter();
  const getClient = useConnectionStore((s) => s.getClient);

  const [fileName, setFileName] = useState<string | null>(null);
  const [fileUri, setFileUri] = useState<string | null>(null);
  const [version, setVersion] = useState('');
  const [sha256, setSha256] = useState('');
  const [signature, setSignature] = useState('');
  const [busy, setBusy] = useState(false);
  const [errors, setErrors] = useState<Record<string, string>>({});

  const pickFile = async () => {
    const result = await DocumentPicker.getDocumentAsync({
      type: 'application/octet-stream',
      copyToCacheDirectory: true,
    });
    if (!result.canceled && result.assets.length > 0) {
      setFileName(result.assets[0].name);
      setFileUri(result.assets[0].uri);
    }
  };

  const onUpload = async () => {
    setErrors({});
    if (!fileUri) {
      setErrors({ file: 'Choose a firmware file first.' });
      return;
    }
    const meta = { version: version.trim(), sha256: sha256.trim(), signature: signature.trim() };
    const check = validateOtaMetadata(meta);
    if (!check.ok) {
      setErrors(check.errors as Record<string, string>);
      return;
    }

    const client = getClient();
    if (!client) {
      setErrors({ file: 'Not connected to a device.' });
      return;
    }

    setBusy(true);
    try {
      // Read the picked file into a binary body for the raw upload.
      const fileResponse = await fetch(fileUri);
      const blob = await fileResponse.blob();
      await client.uploadOta(blob, meta);
      Alert.alert('Update uploaded', 'The device verified the image and will reboot.', [
        { text: 'OK', onPress: () => router.back() },
      ]);
    } catch (e) {
      setErrors({ file: (e as Error).message ?? 'Upload failed.' });
    } finally {
      setBusy(false);
    }
  };

  return (
    <Screen>
      <Stack.Screen options={{ headerShown: true, title: 'Firmware update' }} />

      <AppCard title="Firmware image">
        <Text style={styles.body}>
          Select a signed firmware file and paste its version, SHA‑256, and signature from the
          release. The device verifies everything before applying.
        </Text>
        <AppButton
          label={fileName ?? 'Choose firmware file'}
          variant="outline"
          onPress={pickFile}
          testID="pick-file"
        />
        {errors.file ? <Text style={styles.error}>{errors.file}</Text> : null}
      </AppCard>

      <AppCard title="Release metadata">
        <AppTextField
          label="Version"
          value={version}
          onChangeText={setVersion}
          placeholder="2.1.0"
          errorText={errors.version}
          testID="version-input"
        />
        <AppTextField
          label="SHA‑256"
          value={sha256}
          onChangeText={setSha256}
          placeholder="64 hex characters"
          errorText={errors.sha256}
          testID="sha-input"
        />
        <AppTextField
          label="Signature"
          value={signature}
          onChangeText={setSignature}
          placeholder="base64 signature"
          errorText={errors.signature}
          testID="sig-input"
        />
      </AppCard>

      <View style={styles.warn}>
        <Text style={styles.warnText}>
          Don’t power off the controller during the update.
        </Text>
      </View>

      <AppButton label="Upload & apply" onPress={onUpload} loading={busy} testID="upload-button" />
    </Screen>
  );
}

const styles = StyleSheet.create((theme) => ({
  body: {
    fontSize: theme.fontSize.md,
    color: theme.colors.text,
    lineHeight: 22,
  },
  error: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.danger,
  },
  warn: {
    backgroundColor: theme.colors.surface,
    borderRadius: theme.radius.md,
    borderWidth: 1,
    borderColor: theme.colors.warning,
    padding: theme.space.md,
  },
  warnText: {
    fontSize: theme.fontSize.md,
    color: theme.colors.text,
  },
}));
