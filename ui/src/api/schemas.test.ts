import {
  channelConfigSchema,
  deviceConfigSchema,
  thresholdsSchema,
  timeWindowSchema,
  RelayPolarity,
  MAX_MINUTE_OF_DAY,
} from '@/api/schemas';

describe('schema validators (mirror firmware ApiValidation.h)', () => {
  describe('timeWindowSchema', () => {
    it('accepts in-range minutes', () => {
      expect(timeWindowSchema.safeParse({ start: 0, end: MAX_MINUTE_OF_DAY }).success).toBe(true);
    });
    it('rejects minute > 1439', () => {
      expect(timeWindowSchema.safeParse({ start: 0, end: 1440 }).success).toBe(false);
    });
    it('rejects negative minute', () => {
      expect(timeWindowSchema.safeParse({ start: -1, end: 10 }).success).toBe(false);
    });
  });

  describe('thresholdsSchema', () => {
    it('accepts ordered, in-range thresholds', () => {
      expect(
        thresholdsSchema.safeParse({ lowC: 20, highC: 31, hysteresisC: 0.5 }).success,
      ).toBe(true);
    });
    it('rejects low >= high', () => {
      expect(thresholdsSchema.safeParse({ lowC: 31, highC: 20, hysteresisC: 0.5 }).success).toBe(
        false,
      );
    });
    it('rejects out-of-range temps', () => {
      expect(thresholdsSchema.safeParse({ lowC: -20, highC: 70, hysteresisC: 0.5 }).success).toBe(
        false,
      );
    });
    it('rejects hysteresis > 5', () => {
      expect(thresholdsSchema.safeParse({ lowC: 20, highC: 31, hysteresisC: 6 }).success).toBe(
        false,
      );
    });
  });

  describe('channelConfigSchema', () => {
    const base = {
      name: 'Lights',
      enabled: true,
      cutOnOverTemp: false,
      polarity: RelayPolarity.ActiveLow,
      schedule: { inverted: false, windows: [{ start: 480, end: 1080 }] },
    };
    it('accepts a valid channel', () => {
      expect(channelConfigSchema.safeParse(base).success).toBe(true);
    });
    it('rejects an empty name', () => {
      expect(channelConfigSchema.safeParse({ ...base, name: '' }).success).toBe(false);
    });
    it('rejects a non-ASCII name', () => {
      expect(channelConfigSchema.safeParse({ ...base, name: 'Tÿpe' }).success).toBe(false);
    });
    it('rejects > 8 windows', () => {
      const windows = Array.from({ length: 9 }, () => ({ start: 0, end: 1 }));
      expect(
        channelConfigSchema.safeParse({ ...base, schedule: { inverted: false, windows } }).success,
      ).toBe(false);
    });
  });

  describe('deviceConfigSchema', () => {
    it('rejects > 16 channels', () => {
      const channels = Array.from({ length: 17 }, () => ({
        name: 'C',
        enabled: true,
        cutOnOverTemp: false,
        polarity: RelayPolarity.ActiveHigh,
        schedule: { inverted: false, windows: [] },
      }));
      expect(
        deviceConfigSchema.safeParse({
          tz: 'UTC0',
          thresholds: { lowC: 20, highC: 31, hysteresisC: 0.5 },
          channels,
        }).success,
      ).toBe(false);
    });
  });
});
