import type { DeviceStatus } from './schemas';

/**
 * Decoders that turn the firmware's numeric status fields into
 * human-readable, display-ready values.
 *
 * Enum values MIRROR the firmware:
 *  - WifiState  (network/WiFiManager.h): 0 unprovisioned, 1 connecting, 2 connected, 3 backoff
 *  - TimeSource (time/TimeService.h):    0 NTP, 1 RTC, 2 soft-clock
 *  - Alert bits (alert/AlertTypes.h):    0 high temp, 1 low temp, 2 sensor fault,
 *                                        3 RTC failure, 4 NTP failure
 */

export enum WifiState {
  Unprovisioned = 0,
  Connecting = 1,
  Connected = 2,
  Backoff = 3,
}

export enum TimeSource {
  Ntp = 0,
  Rtc = 1,
  SoftClock = 2,
}

export enum AlertBit {
  HighTemp = 0,
  LowTemp = 1,
  SensorFault = 2,
  RtcFailure = 3,
  NtpFailure = 4,
}

export type AlertSeverity = 'info' | 'warning' | 'critical';

export interface DecodedAlert {
  bit: AlertBit;
  label: string;
  severity: AlertSeverity;
}

const ALERT_META: Record<AlertBit, { label: string; severity: AlertSeverity }> = {
  [AlertBit.HighTemp]: { label: 'High temperature', severity: 'critical' },
  [AlertBit.LowTemp]: { label: 'Low temperature', severity: 'warning' },
  [AlertBit.SensorFault]: { label: 'Sensor fault', severity: 'critical' },
  [AlertBit.RtcFailure]: { label: 'Clock (RTC) failure', severity: 'warning' },
  [AlertBit.NtpFailure]: { label: 'Time sync (NTP) failure', severity: 'info' },
};

export function wifiStateLabel(state: number): string {
  switch (state) {
    case WifiState.Unprovisioned:
      return 'Not provisioned';
    case WifiState.Connecting:
      return 'Connecting…';
    case WifiState.Connected:
      return 'Connected';
    case WifiState.Backoff:
      return 'Reconnecting…';
    default:
      return 'Unknown';
  }
}

export function timeSourceLabel(source: number): string {
  switch (source) {
    case TimeSource.Ntp:
      return 'Internet (NTP)';
    case TimeSource.Rtc:
      return 'Real-time clock';
    case TimeSource.SoftClock:
      return 'Soft clock (no sync)';
    default:
      return 'Unknown';
  }
}

/** Expand the alert bitmask into the list of active alerts (worst-first). */
export function decodeAlerts(mask: number): DecodedAlert[] {
  const order: AlertSeverity[] = ['critical', 'warning', 'info'];
  const active: DecodedAlert[] = [];
  for (const key of Object.keys(ALERT_META) as unknown as AlertBit[]) {
    const bit = Number(key) as AlertBit;
    if ((mask & (1 << bit)) !== 0) {
      active.push({ bit, ...ALERT_META[bit] });
    }
  }
  return active.sort((a, b) => order.indexOf(a.severity) - order.indexOf(b.severity));
}

/** Convenience: highest active severity, or null when all-clear. */
export function worstSeverity(mask: number): AlertSeverity | null {
  const alerts = decodeAlerts(mask);
  return alerts.length > 0 ? alerts[0].severity : null;
}

/** Format the device's minute-of-day as a zero-padded HH:MM string. */
export function formatMinuteOfDay(minute: number): string {
  const clamped = Math.max(0, Math.min(1439, Math.trunc(minute)));
  const h = Math.floor(clamped / 60);
  const m = clamped % 60;
  return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}`;
}

/** Temperature for display: respects the validity flag and the -127 fault. */
export function formatTemperature(status: Pick<DeviceStatus, 'tempC' | 'tempValid'>): string {
  if (!status.tempValid || status.tempC <= -100) {
    return '—';
  }
  return `${status.tempC.toFixed(1)}°C`;
}
