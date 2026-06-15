module.exports = function (api) {
  api.cache(true);
  return {
    presets: ['babel-preset-expo'],
    plugins: [
      // Unistyles processes components at build time; point it at our source root.
      [
        'react-native-unistyles/plugin',
        {
          root: 'src',
        },
      ],
      // react-native-reanimated/react-native-worklets plugin MUST be last.
      'react-native-worklets/plugin',
    ],
  };
};
