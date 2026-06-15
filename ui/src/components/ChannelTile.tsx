import { Text, View } from 'react-native';
import { StyleSheet } from 'react-native-unistyles';

/**
 * A single channel state tile: name + ON/OFF indicator. `name` may be omitted
 * when the live status arrives before config (then it shows the index).
 */
export interface ChannelTileProps {
  index: number;
  name?: string;
  energized: boolean;
}

export function ChannelTile({ index, name, energized }: ChannelTileProps) {
  styles.useVariants({ on: energized });
  return (
    <View style={styles.tile}>
      <View style={styles.dot} />
      <View style={styles.body}>
        <Text style={styles.name} numberOfLines={1}>
          {name ?? `Channel ${index + 1}`}
        </Text>
        <Text style={styles.state}>{energized ? 'ON' : 'OFF'}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create((theme) => ({
  tile: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: theme.space.md,
    backgroundColor: theme.colors.background,
    borderRadius: theme.radius.md,
    borderWidth: 1,
    borderColor: theme.colors.border,
    padding: theme.space.md,
    minWidth: 150,
    flexGrow: 1,
    flexBasis: '45%',
  },
  dot: {
    width: 12,
    height: 12,
    borderRadius: theme.radius.pill,
    variants: {
      on: {
        true: { backgroundColor: theme.colors.success },
        false: { backgroundColor: theme.colors.border },
      },
    },
  },
  body: {
    flex: 1,
  },
  name: {
    fontSize: theme.fontSize.md,
    fontWeight: '600',
    color: theme.colors.text,
  },
  state: {
    fontSize: theme.fontSize.sm,
    variants: {
      on: {
        true: { color: theme.colors.success },
        false: { color: theme.colors.textMuted },
      },
    },
  },
}));
