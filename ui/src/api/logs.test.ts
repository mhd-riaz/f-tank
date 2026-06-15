import { decodeLog, logTime, LogEventType } from '@/api/logs';
import type { LogRecord } from '@/api/schemas';

function rec(partial: Partial<LogRecord>): LogRecord {
  return { epoch: 0, min: 600, type: 0, arg: 0, ...partial };
}

describe('logTime', () => {
  it('uses minute-of-day when epoch is untrusted (0)', () => {
    expect(logTime(rec({ epoch: 0, min: 545 }))).toBe('09:05');
  });
  it('uses a real clock when epoch is set', () => {
    const out = logTime(rec({ epoch: 1_700_000_000, min: 0 }));
    expect(out).not.toBe('00:00'); // formatted as a locale date/time string
  });
});

describe('decodeLog', () => {
  it('decodes boot', () => {
    const d = decodeLog(rec({ type: LogEventType.Boot, arg: 1 }));
    expect(d.title).toBe('Device started');
    expect(d.tone).toBe('info');
  });

  it('decodes channel on/off with names', () => {
    const names = (i: number) => ['Lights', 'Heater'][i];
    expect(decodeLog(rec({ type: LogEventType.ChannelOn, arg: 0 }), names).title).toBe(
      'Lights turned ON',
    );
    expect(decodeLog(rec({ type: LogEventType.ChannelOff, arg: 1 }), names).title).toBe(
      'Heater turned OFF',
    );
  });

  it('falls back to a generic channel name', () => {
    expect(decodeLog(rec({ type: LogEventType.ChannelOn, arg: 2 })).title).toBe(
      'Channel 3 turned ON',
    );
  });

  it('decodes temperature with a value', () => {
    const d = decodeLog(rec({ type: LogEventType.Temperature, tempC: 26.5 }));
    expect(d.title).toBe('Temperature');
    expect(d.detail).toBe('26.5°C');
  });

  it('decodes an alert set with the mask described', () => {
    // bit 0 = High temp, bit 2 = Sensor fault
    const d = decodeLog(rec({ type: LogEventType.AlertSet, detail: 0b101 }));
    expect(d.title).toBe('Alert raised');
    expect(d.detail).toMatch(/High temperature/);
    expect(d.detail).toMatch(/Sensor fault/);
    expect(d.tone).toBe('danger');
  });

  it('decodes a time-source change', () => {
    const d = decodeLog(rec({ type: LogEventType.TimeSource, arg: 0 }));
    expect(d.detail).toBe('Internet (NTP)');
  });

  it('decodes config applied', () => {
    expect(decodeLog(rec({ type: LogEventType.ConfigApplied })).title).toBe(
      'Configuration applied',
    );
  });
});
