import { StyleSheet } from 'react-native-unistyles';

/**
 * Unistyles design-system configuration for the f-tank app.
 *
 * Defines light/dark theme tokens (colors, spacing, radii, typography) and
 * breakpoints. Must be imported once, before any component renders — see
 * `src/app/_layout.tsx`, which imports this module at the top.
 */

const sharedTokens = {
  space: {
    xs: 4,
    sm: 8,
    md: 12,
    lg: 16,
    xl: 24,
    xxl: 32,
  },
  radius: {
    sm: 6,
    md: 10,
    lg: 16,
    pill: 999,
  },
  fontSize: {
    sm: 13,
    md: 15,
    lg: 18,
    xl: 24,
    xxl: 32,
  },
} as const;

const lightTheme = {
  ...sharedTokens,
  colors: {
    background: '#ffffff',
    surface: '#f2f4f7',
    text: '#0b1f33',
    textMuted: '#5b6b7b',
    primary: '#208aef',
    primaryText: '#ffffff',
    border: '#d7dee6',
    success: '#1f9d57',
    warning: '#d98a00',
    danger: '#d23b3b',
  },
} as const;

const darkTheme = {
  ...sharedTokens,
  colors: {
    background: '#0b1220',
    surface: '#16202e',
    text: '#e8eef5',
    textMuted: '#9fb0c0',
    primary: '#4aa3ff',
    primaryText: '#04121f',
    border: '#243243',
    success: '#3fc97e',
    warning: '#f0a92b',
    danger: '#ff5d5d',
  },
} as const;

const appThemes = {
  light: lightTheme,
  dark: darkTheme,
};

const breakpoints = {
  xs: 0,
  sm: 360,
  md: 600,
  lg: 900,
} as const;

type AppThemes = typeof appThemes;
type AppBreakpoints = typeof breakpoints;

declare module 'react-native-unistyles' {
  // eslint-disable-next-line @typescript-eslint/no-empty-object-type
  export interface UnistylesThemes extends AppThemes {}
  // eslint-disable-next-line @typescript-eslint/no-empty-object-type
  export interface UnistylesBreakpoints extends AppBreakpoints {}
}

StyleSheet.configure({
  themes: appThemes,
  breakpoints,
  settings: {
    // Follow the OS light/dark preference.
    adaptiveThemes: true,
  },
});
