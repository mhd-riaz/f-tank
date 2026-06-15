import { Switch, Text, View } from 'react-native';
import { StyleSheet, useUnistyles } from 'react-native-unistyles';

/** Labeled switch row for boolean settings (enabled, inverted, etc.). */
export interface ToggleRowProps {
  label: string;
  value: boolean;
  onValueChange: (value: boolean) => void;
  description?: string;
  disabled?: boolean;
  testID?: string;
}

export function ToggleRow({
  label,
  value,
  onValueChange,
  description,
  disabled = false,
  testID,
}: ToggleRowProps) {
  const { theme } = useUnistyles();
  return (
    <View style={styles.row}>
      <View style={styles.text}>
        <Text style={styles.label}>{label}</Text>
        {description ? <Text style={styles.description}>{description}</Text> : null}
      </View>
      <Switch
        accessibilityLabel={label}
        value={value}
        onValueChange={onValueChange}
        disabled={disabled}
        trackColor={{ true: theme.colors.primary, false: theme.colors.border }}
        testID={testID}
      />
    </View>
  );
}

const styles = StyleSheet.create((theme) => ({
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    gap: theme.space.md,
  },
  text: {
    flex: 1,
    gap: 2,
  },
  label: {
    fontSize: theme.fontSize.md,
    fontWeight: '600',
    color: theme.colors.text,
  },
  description: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.textMuted,
  },
}));
