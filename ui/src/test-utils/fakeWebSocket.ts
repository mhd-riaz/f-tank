/**
 * Controllable WebSocket stand-in for tests. The RN/jsdom test environment's
 * WebSocket can't be driven deterministically, so tests install this to emit
 * frames / errors on demand. Not shipped in the app bundle.
 */
export class FakeWebSocket {
  static last: FakeWebSocket | null = null;
  static instances: FakeWebSocket[] = [];

  onopen: (() => void) | null = null;
  onmessage: ((e: { data: string }) => void) | null = null;
  onerror: (() => void) | null = null;
  onclose: (() => void) | null = null;
  closed = false;

  constructor(public url: string) {
    FakeWebSocket.last = this;
    FakeWebSocket.instances.push(this);
  }

  emit(data: unknown) {
    this.onmessage?.({ data: JSON.stringify(data) });
  }

  fail() {
    this.onerror?.();
  }

  close() {
    this.closed = true;
    this.onclose?.();
  }
}

/** Install the fake as global WebSocket; returns a restore function. */
export function installFakeWebSocket(): () => void {
  const original = globalThis.WebSocket;
  FakeWebSocket.last = null;
  FakeWebSocket.instances = [];
  // @ts-expect-error - swap in the fake for the test duration
  globalThis.WebSocket = FakeWebSocket;
  return () => {
    globalThis.WebSocket = original;
  };
}
