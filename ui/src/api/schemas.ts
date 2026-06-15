import { z } from 'zod';

/**
 * Device API contract schemas (Zod) — the single source of truth for the
 * shapes the f-tank firmware speaks over its LAN REST API (REQUIREMENTS §7.1).
 *
 * Bounds here MIRROR the firmware validators in
 * `embedded/src/api/ApiValidation.h` + `embedded/src/schedule/Schedule.h`.
 * Keep them in lock-step: the app rejects invalid input locally BEFORE PUT so
 * the user gets instant feedback, and the device re-validates as defense (NFR-8).
 */

// --- Firmware constants (mirror embedded/) ---
export const MAX_CHANNELS = 16; // channel::kMaxChannels
export const MAX_WINDOWS_PER_CHANNEL = 8; // schedule::kMaxWindowsPerChannel
export const MAX_MINUTE_OF_DAY = 1439; // schedule::kMaxMinuteOfDay
export const CHANNEL_NAME_MAX = 23; // kChannelNameSize(24) - 1 for NUL
export const TZ_STRING_MAX = 47; // kTzStringSize(48) - 1 for NUL
export const CONFIG_VERSION = 2; // storage::kConfigVersion

/** Printable-ASCII only, matching the firmware byte check (0x20..0x7E). */
const printableAscii = (max: number) =>
  z
    .string()
    .min(1)
    .max(max)
    .refine((s) => /^[\x20-\x7e]+$/.test(s), {
      message: 'must be printable ASCII',
    });

/** Relay drive polarity — provisioned, read-only (FR-12 / NFR-11). */
export enum RelayPolarity {
  ActiveHigh = 0,
  ActiveLow = 1,
}

export const minuteOfDaySchema = z.number().int().min(0).max(MAX_MINUTE_OF_DAY);

export const timeWindowSchema = z.object({
  start: minuteOfDaySchema,
  end: minuteOfDaySchema,
});

export const channelScheduleSchema = z.object({
  inverted: z.boolean(),
  windows: z.array(timeWindowSchema).max(MAX_WINDOWS_PER_CHANNEL),
});

export const channelConfigSchema = z.object({
  name: printableAscii(CHANNEL_NAME_MAX),
  enabled: z.boolean(),
  // Read-only / provisioned — surfaced for display, never edited by the app.
  cutOnOverTemp: z.boolean(),
  polarity: z.nativeEnum(RelayPolarity),
  schedule: channelScheduleSchema,
});

export const thresholdsSchema = z
  .object({
    lowC: z.number(),
    highC: z.number(),
    hysteresisC: z.number().min(0).max(5),
  })
  .refine((t) => t.lowC < t.highC, { message: 'lowC must be below highC' })
  .refine((t) => t.lowC >= -10 && t.highC <= 60, {
    message: 'outside plausible aquarium range',
  });

export const tzSchema = printableAscii(TZ_STRING_MAX);

export const deviceConfigSchema = z.object({
  tz: tzSchema,
  thresholds: thresholdsSchema,
  channels: z.array(channelConfigSchema).max(MAX_CHANNELS),
});

// --- Read-only response shapes ---

export const infoSchema = z.object({
  firmware: z.string(),
  configVersion: z.number().int(),
  channelCount: z.number().int().min(0).max(MAX_CHANNELS).optional(),
});

export const statusSchema = z.object({
  tempC: z.number(),
  tempValid: z.boolean(),
  alerts: z.number().int(),
  timeSource: z.number().int(),
  wifiState: z.number().int(),
  minuteOfDay: z.number().int(),
  channels: z.array(z.boolean()),
});

export const healthSchema = z.object({ ok: z.boolean() });

// --- On-demand event log (FR-29). Mirrors firmware serializeLogRecord. ---

export const logRecordSchema = z.object({
  epoch: z.number().int(), // local-time Unix seconds; 0 if time untrusted
  min: z.number().int(), // minute-of-day ordering fallback
  type: z.number().int(), // LogEventType
  arg: z.number().int(), // channel index / alert id / time source
  tempC: z.number().optional(), // present only for temperature records
  detail: z.number().int().optional(), // alert mask / state bitfield (omitted if 0)
});

export const logsResponseSchema = z.object({
  recording: z.boolean(),
  persisted: z.number().int(),
  count: z.number().int(),
  records: z.array(logRecordSchema),
});

// --- Derived TypeScript types ---
export type TimeWindow = z.infer<typeof timeWindowSchema>;
export type ChannelSchedule = z.infer<typeof channelScheduleSchema>;
export type ChannelConfig = z.infer<typeof channelConfigSchema>;
export type Thresholds = z.infer<typeof thresholdsSchema>;
export type DeviceConfig = z.infer<typeof deviceConfigSchema>;
export type DeviceInfo = z.infer<typeof infoSchema>;
export type DeviceStatus = z.infer<typeof statusSchema>;
export type LogRecord = z.infer<typeof logRecordSchema>;
export type LogsResponse = z.infer<typeof logsResponseSchema>;
