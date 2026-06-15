import type { LiveSource } from '@/hooks/useLiveStatus';
import type { PillTone } from '@/components/StatusPill';
import type { AlertSeverity } from '@/api/status';

/** Map the live-status source to a pill label + tone. */
export function liveSourcePill(source: LiveSource): { label: string; tone: PillTone } {
  switch (source) {
    case 'websocket':
      return { label: 'Live', tone: 'success' };
    case 'polling':
      return { label: 'Live (polling)', tone: 'info' };
    case 'connecting':
      return { label: 'Connecting…', tone: 'neutral' };
    case 'offline':
      return { label: 'Offline', tone: 'danger' };
  }
}

/** Map an alert severity to a pill tone. */
export function severityTone(severity: AlertSeverity): PillTone {
  switch (severity) {
    case 'critical':
      return 'danger';
    case 'warning':
      return 'warning';
    case 'info':
      return 'info';
  }
}
