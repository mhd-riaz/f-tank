// Ambient module shims for untyped transitive dependencies that TypeScript
// reaches via package "react-native" export conditions (their JS ships without
// declarations). Runtime is unaffected — Metro transpiles them normally.
declare module '@react-native/normalize-colors';
