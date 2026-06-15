import { decodeAlerts, formatMinuteOfDay, timeSourceLabel } from './status';
import type { LogRecord } from './schemas';

/**
 * Decoders for on-demand event-log records (FR-29). Event types mirror the
 * firmware `LogEventType` (log/LogTypes.h):
 *   1 boot, 2 channelOn, 3 channelOff, 4 temperature, 5 alertSet,
 *   6 alertClear, 7 timeSource, 8 configApplied.
 */

export enum LogEventType {
  None = 0,
  Boot = 1,
  ChannelOn = 2,
  ChannelOff = 3,
  Temperature = 4,
  AlertSet = 5,
  AlertClear = 6,
  TimeSource = 7,
  ConfigApplied = 8,
}

export type LogTone = 'neutral' | 'success' | 'warning' | 'danger' | 'info';

export interface DecodedLog {
  /** Stable-ish key for lists (epoch+min+type+arg). */
  key: string;
  /** Short event title. */
  title: string;
  /** Optional secondary detail. */
  detail?: string;
  /** Display time: clock from epoch if trusted, else minute-of-day. */
  time: string;
  tone: LogTone;
}

function describeAlertMask(mask: number): string {
  const alerts = decodeAlerts(mask);
  return alerts.length === 0 ? 'all clear' : alerts.map((a) => a.label).join(', ');
}

/** Display time for a record: real clock if epoch is trusted, else HH:MM. */
export function logTime(record: LogRecord): string {
  if (record.epoch > 0) {
    return new Date(record.epoch * 1000).toLocaleString();
  }
  return formatMinuteOfDay(record.min);
}

export function decodeLog(record: LogRecord, channelName?: (i: number) => string): DecodedLog {
  const key = `${record.epoch}-${record.min}-${record.type}-${record.arg}`;
  const time = logTime(record);
  const name = (i: number) => channelName?.(i) ?? `Channel ${i + 1}`;

  switch (record.type) {
    case LogEventType.Boot:
      return { key, title: 'Device started', detail: `boot reason ${record.arg}`, time, tone: 'info' };
    case LogEventType.ChannelOn:
      return { key, title: `${name(record.arg)} turned ON`, time, tone: 'success' };
    case LogEventType.ChannelOff:
      return { key, title: `${name(record.arg)} turned OFF`, time, tone: 'neutral' };
    case LogEventType.Temperature:
      return {
        key,
        title: 'Temperature',
        detail: record.tempC !== undefined ? `${record.tempC.toFixed(1)}°C` : undefined,
        time,
        tone: 'info',
      };
    case LogEventType.AlertSet:
      return {
        key,
        title: 'Alert raised',
        detail: describeAlertMask(record.detail ?? 0),
        time,
        tone: 'danger',
      };
    case LogEventType.AlertClear:
      return {
        key,
        title: 'Alert cleared',
        detail: describeAlertMask(record.detail ?? 0),
        time,
        tone: 'success',
      };
    case LogEventType.TimeSource:
      return {
        key,
        title: 'Time source changed',
        detail: timeSourceLabel(record.arg),
        time,
        tone: 'info',
      };
    case LogEventType.ConfigApplied:
      return { key, title: 'Configuration applied', time, tone: 'info' };
    default:
      return { key, title: `Event ${record.type}`, time, tone: 'neutral' };
  }
}
