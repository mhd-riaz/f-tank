import { Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

/**
 * Small colored status pill used for connection state, alert severity, etc.
 */
export type PillTone = 'neutral' | 'success' | 'warning' | 'danger' | 'info';

export interface StatusPillProps {
  label: string;
  tone?: PillTone;
}

export function StatusPill({ label, tone = 'neutral' }: StatusPillProps) {
  styles.useVariants({ tone });
  return (
    <View style={styles.pill}>
      <Text style={styles.text}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create((theme) => ({
  pill: {
    alignSelf: 'flex-start',
    borderRadius: theme.radius.pill,
    paddingHorizontal: theme.space.md,
    paddingVertical: theme.space.xs,
    variants: {
      tone: {
        neutral: { backgroundColor: theme.colors.border },
        success: { backgroundColor: theme.colors.success },
        warning: { backgroundColor: theme.colors.warning },
        danger: { backgroundColor: theme.colors.danger },
        info: { backgroundColor: theme.colors.primary },
      },
    },
  },
  text: {
    fontSize: theme.fontSize.sm,
    fontWeight: '600',
    variants: {
      tone: {
        neutral: { color: theme.colors.text },
        success: { color: '#ffffff' },
        warning: { color: '#1a1205' },
        danger: { color: '#ffffff' },
        info: { color: theme.colors.primaryText },
      },
    },
  },
}));
