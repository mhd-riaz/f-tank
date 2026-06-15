import { Text, TextInput, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

/**
 * Labeled text field with optional helper/error text. Thin wrapper over RN
 * TextInput so forms (connect, schedules, thresholds) stay consistent and
 * accessible.
 */
export interface AppTextFieldProps {
  label: string;
  value: string;
  onChangeText: (text: string) => void;
  placeholder?: string;
  helperText?: string;
  errorText?: string;
  autoCapitalize?: 'none' | 'sentences' | 'words' | 'characters';
  keyboardType?: 'default' | 'numeric' | 'url';
  autoCorrect?: boolean;
  secureTextEntry?: boolean;
  testID?: string;
}

export function AppTextField({
  label,
  value,
  onChangeText,
  placeholder,
  helperText,
  errorText,
  autoCapitalize = 'none',
  keyboardType = 'default',
  autoCorrect = false,
  secureTextEntry = false,
  testID,
}: AppTextFieldProps) {
  const hasError = Boolean(errorText);
  styles.useVariants({ error: hasError });

  return (
    <View style={styles.container}>
      <Text style={styles.label}>{label}</Text>
      <TextInput
        accessibilityLabel={label}
        value={value}
        onChangeText={onChangeText}
        placeholder={placeholder}
        placeholderTextColor={styles.placeholderColor.color}
        autoCapitalize={autoCapitalize}
        keyboardType={keyboardType}
        autoCorrect={autoCorrect}
        secureTextEntry={secureTextEntry}
        style={styles.input}
        testID={testID}
      />
      {hasError ? (
        <Text style={styles.error}>{errorText}</Text>
      ) : helperText ? (
        <Text style={styles.helper}>{helperText}</Text>
      ) : null}
    </View>
  );
}

const styles = StyleSheet.create((theme) => ({
  container: {
    gap: theme.space.xs,
  },
  label: {
    fontSize: theme.fontSize.sm,
    fontWeight: '600',
    color: theme.colors.text,
  },
  input: {
    borderWidth: 1,
    borderRadius: theme.radius.md,
    paddingHorizontal: theme.space.md,
    paddingVertical: theme.space.md,
    fontSize: theme.fontSize.md,
    color: theme.colors.text,
    backgroundColor: theme.colors.background,
    variants: {
      error: {
        true: { borderColor: theme.colors.danger },
        false: { borderColor: theme.colors.border },
      },
    },
  },
  helper: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.textMuted,
  },
  error: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.danger,
  },
  placeholderColor: {
    color: theme.colors.textMuted,
  },
}));
