import { Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

/** Label/value row used in status and detail cards. */
export interface InfoRowProps {
  label: string;
  value: string;
}

export function InfoRow({ label, value }: InfoRowProps) {
  return (
    <View style={styles.row}>
      <Text style={styles.label}>{label}</Text>
      <Text style={styles.value}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create((theme) => ({
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    gap: theme.space.md,
  },
  label: {
    fontSize: theme.fontSize.md,
    color: theme.colors.textMuted,
  },
  value: {
    fontSize: theme.fontSize.md,
    fontWeight: '600',
    color: theme.colors.text,
    flexShrink: 1,
    textAlign: 'right',
  },
}));
