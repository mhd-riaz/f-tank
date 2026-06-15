import {
  addWindow,
  buildConfigUpdate,
  clearSchedule,
  describeWindow,
  formatHhMm,
  parseHhMm,
  removeWindow,
  setInverted,
  validateWindow,
} from '@/api/schedule';
import { RelayPolarity, type ChannelConfig, type DeviceConfig } from '@/api/schemas';

const baseSchedule = { inverted: false, windows: [{ start: 480, end: 1080 }] };

function makeChannel(name: string): ChannelConfig {
  return {
    name,
    enabled: true,
    cutOnOverTemp: false,
    polarity: RelayPolarity.ActiveLow,
    schedule: { inverted: false, windows: [] },
  };
}

describe('parseHhMm', () => {
  it('parses valid times', () => {
    expect(parseHhMm('08:00')).toBe(480);
    expect(parseHhMm('00:00')).toBe(0);
    expect(parseHhMm('23:59')).toBe(1439);
    expect(parseHhMm('9:05')).toBe(545);
  });
  it('rejects invalid times', () => {
    expect(parseHhMm('24:00')).toBeNull();
    expect(parseHhMm('08:60')).toBeNull();
    expect(parseHhMm('8')).toBeNull();
    expect(parseHhMm('abc')).toBeNull();
  });
});

describe('formatHhMm / describeWindow', () => {
  it('formats minutes', () => {
    expect(formatHhMm(545)).toBe('09:05');
    expect(formatHhMm(1439)).toBe('23:59');
  });
  it('describes a normal window', () => {
    expect(describeWindow({ start: 480, end: 1080 })).toBe('08:00 – 18:00');
  });
  it('marks a cross-midnight window', () => {
    expect(describeWindow({ start: 1320, end: 360 })).toBe('22:00 – 06:00 (next day)');
  });
});

describe('validateWindow', () => {
  it('accepts a normal window', () => {
    expect(validateWindow({ start: 480, end: 1080 }).ok).toBe(true);
  });
  it('accepts a cross-midnight window', () => {
    expect(validateWindow({ start: 1320, end: 360 }).ok).toBe(true);
  });
  it('rejects equal start/end', () => {
    expect(validateWindow({ start: 480, end: 480 }).ok).toBe(false);
  });
  it('rejects out-of-range', () => {
    expect(validateWindow({ start: -1, end: 100 }).ok).toBe(false);
    expect(validateWindow({ start: 0, end: 1440 }).ok).toBe(false);
  });
});

describe('addWindow', () => {
  it('appends a valid window', () => {
    const result = addWindow(baseSchedule, { start: 1200, end: 1260 });
    expect(result.ok).toBe(true);
    expect(result.schedule.windows).toHaveLength(2);
  });
  it('rejects an invalid window', () => {
    const result = addWindow(baseSchedule, { start: 5, end: 5 });
    expect(result.ok).toBe(false);
    expect(result.schedule.windows).toHaveLength(1);
  });
  it('enforces the 8-window cap', () => {
    const full = { inverted: false, windows: Array.from({ length: 8 }, () => ({ start: 0, end: 1 })) };
    const result = addWindow(full, { start: 100, end: 200 });
    expect(result.ok).toBe(false);
    expect(result.error).toMatch(/8 windows/);
  });
});

describe('removeWindow / setInverted / clearSchedule', () => {
  it('removes a window by index', () => {
    const two = { inverted: false, windows: [{ start: 0, end: 60 }, { start: 120, end: 180 }] };
    expect(removeWindow(two, 0).windows).toEqual([{ start: 120, end: 180 }]);
  });
  it('toggles inverted', () => {
    expect(setInverted(baseSchedule, true).inverted).toBe(true);
  });
  it('clears all windows', () => {
    expect(clearSchedule(baseSchedule).windows).toEqual([]);
  });
});

describe('buildConfigUpdate', () => {
  const config: DeviceConfig = {
    tz: 'UTC0',
    thresholds: { lowC: 20, highC: 31, hysteresisC: 0.5 },
    channels: [makeChannel('Lights'), makeChannel('Heater'), makeChannel('CO2')],
  };

  it('returns the FULL channels array with only the target replaced', () => {
    const edited: ChannelConfig = {
      ...config.channels[1],
      name: 'Heater 2',
      schedule: { inverted: false, windows: [{ start: 600, end: 720 }] },
    };
    const update = buildConfigUpdate(config, 1, edited);
    expect(update.channels).toHaveLength(3); // satisfies count-match rule
    expect(update.channels[0].name).toBe('Lights'); // others untouched
    expect(update.channels[1].name).toBe('Heater 2');
    expect(update.channels[2].name).toBe('CO2');
  });

  it('preserves read-only fields from the original channel', () => {
    const original = config.channels[0];
    const edited: ChannelConfig = { ...original, name: 'Renamed' };
    const update = buildConfigUpdate(config, 0, edited);
    expect(update.channels[0].polarity).toBe(original.polarity);
    expect(update.channels[0].cutOnOverTemp).toBe(original.cutOnOverTemp);
  });
});
