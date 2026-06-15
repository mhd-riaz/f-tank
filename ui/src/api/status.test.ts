import {
  decodeAlerts,
  formatMinuteOfDay,
  formatTemperature,
  timeSourceLabel,
  wifiStateLabel,
  worstSeverity,
  AlertBit,
} from '@/api/status';

describe('status decoders (mirror firmware enums)', () => {
  it('labels wifi states', () => {
    expect(wifiStateLabel(2)).toBe('Connected');
    expect(wifiStateLabel(0)).toBe('Not provisioned');
    expect(wifiStateLabel(99)).toBe('Unknown');
  });

  it('labels time sources', () => {
    expect(timeSourceLabel(0)).toBe('Internet (NTP)');
    expect(timeSourceLabel(1)).toBe('Real-time clock');
    expect(timeSourceLabel(2)).toBe('Soft clock (no sync)');
  });

  describe('decodeAlerts', () => {
    it('returns empty for an all-clear mask', () => {
      expect(decodeAlerts(0)).toEqual([]);
    });

    it('decodes a single bit', () => {
      const alerts = decodeAlerts(1 << AlertBit.HighTemp);
      expect(alerts).toHaveLength(1);
      expect(alerts[0].label).toBe('High temperature');
      expect(alerts[0].severity).toBe('critical');
    });

    it('orders critical before warning before info', () => {
      const mask =
        (1 << AlertBit.NtpFailure) | (1 << AlertBit.LowTemp) | (1 << AlertBit.SensorFault);
      const order = decodeAlerts(mask).map((a) => a.severity);
      expect(order).toEqual(['critical', 'warning', 'info']);
    });
  });

  it('reports the worst severity', () => {
    expect(worstSeverity(0)).toBeNull();
    expect(worstSeverity(1 << AlertBit.LowTemp)).toBe('warning');
    expect(worstSeverity((1 << AlertBit.LowTemp) | (1 << AlertBit.HighTemp))).toBe('critical');
  });

  describe('formatMinuteOfDay', () => {
    it('formats midnight, noon, and end-of-day', () => {
      expect(formatMinuteOfDay(0)).toBe('00:00');
      expect(formatMinuteOfDay(720)).toBe('12:00');
      expect(formatMinuteOfDay(1439)).toBe('23:59');
    });
    it('clamps out-of-range input', () => {
      expect(formatMinuteOfDay(-5)).toBe('00:00');
      expect(formatMinuteOfDay(5000)).toBe('23:59');
    });
  });

  describe('formatTemperature', () => {
    it('formats a valid reading', () => {
      expect(formatTemperature({ tempC: 25.43, tempValid: true })).toBe('25.4°C');
    });
    it('shows a dash for invalid or fault readings', () => {
      expect(formatTemperature({ tempC: 25, tempValid: false })).toBe('—');
      expect(formatTemperature({ tempC: -127, tempValid: true })).toBe('—');
    });
  });
});
