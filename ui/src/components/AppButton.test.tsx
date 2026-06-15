import { fireEvent, render } from '@testing-library/react-native';

import { AppButton } from '@/components/AppButton';

describe('AppButton', () => {
  it('renders its label and fires onPress', () => {
    const onPress = jest.fn();
    const { getByText, getByTestId } = render(
      <AppButton label="Connect" onPress={onPress} testID="btn" />,
    );

    expect(getByText('Connect')).toBeTruthy();
    fireEvent.press(getByTestId('btn'));
    expect(onPress).toHaveBeenCalledTimes(1);
  });

  it('does not fire onPress when disabled', () => {
    const onPress = jest.fn();
    const { getByTestId } = render(
      <AppButton label="Connect" onPress={onPress} disabled testID="btn" />,
    );

    fireEvent.press(getByTestId('btn'));
    expect(onPress).not.toHaveBeenCalled();
  });

  it('shows a spinner and suppresses onPress while loading', () => {
    const onPress = jest.fn();
    const { queryByText, getByTestId } = render(
      <AppButton label="Connect" onPress={onPress} loading testID="btn" />,
    );

    expect(queryByText('Connect')).toBeNull();
    fireEvent.press(getByTestId('btn'));
    expect(onPress).not.toHaveBeenCalled();
  });
});
