import { ActivityIndicator, Pressable, Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

/**
 * Shared primary/secondary button built on RN primitives + Unistyles.
 *
 * The app is form/action-heavy, so a single themed, accessible button keeps the
 * screens consistent. Variants map to design tokens defined in
 * `src/theme/unistyles.ts`.
 */
export type ButtonVariant = 'primary' | 'outline';

export interface AppButtonProps {
  label: string;
  onPress?: () => void;
  variant?: ButtonVariant;
  disabled?: boolean;
  loading?: boolean;
  testID?: string;
}

export function AppButton({
  label,
  onPress,
  variant = 'primary',
  disabled = false,
  loading = false,
  testID,
}: AppButtonProps) {
  const isDisabled = disabled || loading;
  styles.useVariants({ variant, disabled: isDisabled });

  return (
    <Pressable
      accessibilityRole="button"
      accessibilityState={{ disabled: isDisabled, busy: loading }}
      disabled={isDisabled}
      onPress={onPress}
      testID={testID}
      style={styles.pressable}
    >
      <View style={styles.content}>
        {loading ? (
          <ActivityIndicator color={variant === 'primary' ? '#ffffff' : undefined} />
        ) : (
          <Text style={styles.label}>{label}</Text>
        )}
      </View>
    </Pressable>
  );
}

const styles = StyleSheet.create((theme) => ({
  pressable: {
    width: '100%',
    borderRadius: theme.radius.md,
    variants: {
      variant: {
        primary: { backgroundColor: theme.colors.primary },
        outline: {
          backgroundColor: 'transparent',
          borderWidth: 1,
          borderColor: theme.colors.border,
        },
      },
      disabled: {
        true: { opacity: 0.5 },
        false: { opacity: 1 },
      },
    },
  },
  content: {
    paddingVertical: theme.space.md,
    paddingHorizontal: theme.space.lg,
    alignItems: 'center',
    justifyContent: 'center',
  },
  label: {
    fontSize: theme.fontSize.md,
    fontWeight: '600',
    variants: {
      variant: {
        primary: { color: theme.colors.primaryText },
        outline: { color: theme.colors.text },
      },
      disabled: {},
    },
  },
}));
