const expoPreset = require('jest-expo/jest-preset');

/** @type {import('jest').Config} */
module.exports = {
  ...expoPreset,
  setupFilesAfterEnv: ['<rootDir>/jest.setup.ts'],
  // Add .mjs so MSW v2's ESM dependency chain resolves.
  moduleFileExtensions: ['ts', 'tsx', 'js', 'jsx', 'mjs', 'json', 'node'],
  // Preserve jest-expo's transforms (they pass the metro/native babel caller —
  // dropping it makes babel-preset-expo emit the web build and breaks RN
  // rendering). Only ADD a .mjs entry using the same native caller.
  transform: {
    ...expoPreset.transform,
    '^.+\\.mjs$': ['babel-jest', { caller: { name: 'metro', bundler: 'metro', platform: 'ios' } }],
  },
  // RN/Expo ship untranspiled ESM; let Babel process them. MSW v2 and its
  // dependency chain are ESM-only and must also be transformed.
  transformIgnorePatterns: [
    'node_modules/(?!((jest-)?react-native|@react-native(-community)?|expo(nent)?|@expo(nent)?/.*|@expo-google-fonts/.*|react-navigation|@react-navigation/.*|@unimodules/.*|unimodules|sentry-expo|native-base|react-native-svg|react-native-unistyles|react-native-nitro-modules|react-native-edge-to-edge|@tanstack/.*|zustand|msw|@mswjs/.*|@bundled-es-modules/.*|rettime|until-async|strict-event-emitter|@open-draft/.*|headers-polyfill|outvariant|is-node-process|tough-cookie))',
  ],
  moduleNameMapper: {
    '^@/(.*)$': '<rootDir>/src/$1',
    '^@/assets/(.*)$': '<rootDir>/assets/$1',
    // MSW v2's package exports set the react-native condition to null, which
    // jest-expo's resolver picks; map subpaths to the CommonJS builds instead.
    '^msw/node$': '<rootDir>/node_modules/msw/lib/node/index.js',
    '^msw$': '<rootDir>/node_modules/msw/lib/core/index.js',
  },
  testMatch: ['**/*.test.ts', '**/*.test.tsx'],
  collectCoverageFrom: ['src/**/*.{ts,tsx}', '!src/**/*.d.ts', '!src/mocks/**', '!src/test-utils/**'],
};
