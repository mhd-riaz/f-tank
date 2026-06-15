import { MAX_WINDOWS_PER_CHANNEL } from './schemas';
import type { ChannelConfig, DeviceConfig, TimeWindow } from './schemas';

/**
 * Pure helpers for editing channel schedules. Kept hardware-free and immutable
 * so the schedule UI logic is unit-tested without rendering.
 *
 * Firmware rules mirrored here (ApiValidation.h / Schedule.h):
 *  - minutes-of-day are 0–1439; a window may cross midnight (start > end).
 *  - at most 8 windows per channel.
 *  - PUT /config requires the FULL channels array (length == channelCount); the
 *    device preserves read-only polarity/cut-target, so `buildConfigUpdate`
 *    echoes every channel and replaces just the edited one.
 */

const MAX_MIN = 1439;

/** Parse "HH:MM" (24h) into minutes-of-day, or null if invalid. */
export function parseHhMm(text: string): number | null {
  const m = /^(\d{1,2}):(\d{2})$/.exec(text.trim());
  if (!m) {
    return null;
  }
  const hours = Number(m[1]);
  const mins = Number(m[2]);
  if (hours > 23 || mins > 59) {
    return null;
  }
  return hours * 60 + mins;
}

/** Format minutes-of-day as zero-padded "HH:MM". */
export function formatHhMm(minute: number): string {
  const clamped = Math.max(0, Math.min(MAX_MIN, Math.trunc(minute)));
  const h = Math.floor(clamped / 60);
  const m = clamped % 60;
  return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}`;
}

/** Human description of a window, noting cross-midnight spans. */
export function describeWindow(win: TimeWindow): string {
  const base = `${formatHhMm(win.start)} – ${formatHhMm(win.end)}`;
  return win.start > win.end ? `${base} (next day)` : base;
}

export interface WindowValidation {
  ok: boolean;
  error?: string;
}

/** Validate a single window against firmware bounds. */
export function validateWindow(win: TimeWindow): WindowValidation {
  if (!Number.isInteger(win.start) || !Number.isInteger(win.end)) {
    return { ok: false, error: 'Times must be whole minutes' };
  }
  if (win.start < 0 || win.start > MAX_MIN || win.end < 0 || win.end > MAX_MIN) {
    return { ok: false, error: 'Times must be between 00:00 and 23:59' };
  }
  if (win.start === win.end) {
    return { ok: false, error: 'Start and end cannot be the same' };
  }
  return { ok: true };
}

/** Append a window (immutably). Fails if at the 8-window cap. */
export function addWindow(
  schedule: ChannelConfig['schedule'],
  win: TimeWindow,
): { ok: boolean; schedule: ChannelConfig['schedule']; error?: string } {
  if (schedule.windows.length >= MAX_WINDOWS_PER_CHANNEL) {
    return { ok: false, schedule, error: `At most ${MAX_WINDOWS_PER_CHANNEL} windows per channel` };
  }
  const check = validateWindow(win);
  if (!check.ok) {
    return { ok: false, schedule, error: check.error };
  }
  return { ok: true, schedule: { ...schedule, windows: [...schedule.windows, win] } };
}

/** Remove the window at `index` (immutably). */
export function removeWindow(
  schedule: ChannelConfig['schedule'],
  index: number,
): ChannelConfig['schedule'] {
  return { ...schedule, windows: schedule.windows.filter((_, i) => i !== index) };
}

/** Toggle the channel's inverted (OFF-window) flag. */
export function setInverted(
  schedule: ChannelConfig['schedule'],
  inverted: boolean,
): ChannelConfig['schedule'] {
  return { ...schedule, inverted };
}

/** Empty a channel's windows (reset schedule — no dedicated endpoint). */
export function clearSchedule(
  schedule: ChannelConfig['schedule'],
): ChannelConfig['schedule'] {
  return { ...schedule, windows: [] };
}

/**
 * Build a full-config PUT body that replaces a single channel. The firmware
 * rejects a partial channels array ("channel count mismatch"), so we echo all
 * channels and swap in the edited one. Read-only fields on the edited channel
 * are carried through unchanged from the original config.
 */
export function buildConfigUpdate(
  config: DeviceConfig,
  channelIndex: number,
  nextChannel: ChannelConfig,
): Pick<DeviceConfig, 'channels'> {
  const channels = config.channels.map((c, i) => (i === channelIndex ? nextChannel : c));
  return { channels };
}
