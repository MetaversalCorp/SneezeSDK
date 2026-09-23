# IO

One Socket.IO connection, opened from [`NETWORK`](NETWORK.md). The **host** runs official `socket.io-client`. The guest sees a thin copyable handle (`HIO` / `twIoIx`), two receive queues (events and acks), and five [`INSTANCE`](INSTANCE.md) hooks.

`IO` is not [`SOCKET`](SOCKET.md). `SOCKET` is a browser-shaped WebSocket. `IO` is the Socket.IO application protocol. A Socket.IO server will not treat `Socket_Send_Text` as an `emit`.

## The shape of a connection

```rust
impl INSTANCE for MY_MODULE
{
   fn Open (pHost: &HOST)
   {
      let pIo = pHost.Network ().Io_Open ("https://example.com");

      let _ = pIo;
   }

   fn Io_Opened (pHost: &HOST, pIo: IO)
   {
      pIo.Emit_Text ("testapi", "{\"hello\":true}");
      pIo.Emit_Text_Ex ("testapi", "{\"hello\":true}", 1);
   }

   fn Io_Received (pHost: &HOST, pIo: IO, bBinary: bool, nSize: i64)
   {
      let _ = (bBinary, nSize);

      pHost.Console ().Log (&pIo.Recv_Event ());
      pHost.Console ().Log (&pIo.Recv_Text ());
   }

   fn Io_Acked (pHost: &HOST, pIo: IO, qwParam: u64, nSize: i64)
   {
      let _ = (qwParam, nSize);

      pHost.Console ().Log (&pIo.Recv_Ack_Text ());
   }

   fn Io_Failed (pHost: &HOST, pIo: IO)
   {
      pHost.Console ().Error (&pIo.Error ());
   }

   fn Io_Closed (pHost: &HOST, pIo: IO, wCode: i32, bClean: bool)
   {
      let _ = (wCode, bClean);

      pIo.Free ();
   }
}
```

Rules, parallel to [`SOCKET`](SOCKET.md):

1. **You cannot emit until it opens.** Emit before [`Io_Opened`](INSTANCE.md#io_opened) is refused.
2. **Every `Io_Received` owes `Recv_Event` then `Recv`.** `Recv` pops the event payload. Skip `Recv_Event` and the name is lost with the pop.
3. **Every `Io_Acked` owes `Recv_Ack`.** `qwParam` is already on the notify (the cookie from `Emit_*_Ex`).
4. **`Close` and `Free` are different.** `Close` is Socket.IO `disconnect` and leaves the handle readable. `Free` is the mirror of [`NETWORK::Io_Open`](NETWORK.md#io_open).

`Emit_*` is fire-and-forget. `Emit_*_Ex` always requests an ack; `qwParam` 0 is a valid cookie, not “no ack.”

Built-in Socket.IO events (`connect`, `disconnect`, `connect_error`) stay off both queues. They are Opened / Closed / Failed. An ack callback is not `onAny`.

## State

[`State`](#state) uses the same four values as [`SOCKET`](SOCKET.md#state):

| State | Meaning |
|-------|---------|
| `kSNEEZE_ABI_IO_STATE_CONNECTING` | the handshake is in progress |
| `kSNEEZE_ABI_IO_STATE_OPEN` | emit and receive are live |
| `kSNEEZE_ABI_IO_STATE_CLOSING` | disconnect is in progress |
| `kSNEEZE_ABI_IO_STATE_CLOSED` | the connection is finished |

[`Io_Failed`](INSTANCE.md#io_failed) is always followed by [`Io_Closed`](INSTANCE.md#io_closed) with code `1006`.

## Two queues

Events from `socket.onAny` go on the **event** queue. Acks from `Emit_*_Ex` go on the **ack** queue. Both are bounded the same way as the socket receive queue: stop draining and messages are dropped, and [`Error`](#error) says so.

A single payload is capped at **16 MB**.

## Opening

### Io_Open

```rust
pub fn Io_Open (&self, sUrl: &str) -> IO
```

Reached through [`NETWORK`](NETWORK.md). The URL must be an absolute `http://` or `https://` URL (the host may also accept `ws://` or `wss://`). It is **not** resolved against the fabric.

## Sending

### Emit_Text / Emit_Bytes

Fire-and-forget. Refused unless the connection is `OPEN`.

### Emit_Text_Ex / Emit_Bytes_Ex

Same payload, plus `qwParam`. The host calls `socket.emit(event, payload, ack => ...)`. When the server acks, [`Io_Acked`](INSTANCE.md#io_acked) fires with that `qwParam` and `nSize`. There is no ack timeout in v1.

## Receiving

### Recv_Event

Event name of the **event-queue head**. Does not pop.

### Recv / Recv_Text

Pops the event-queue payload.

### Recv_Ack / Recv_Ack_Text

Pops the ack-queue payload. Call after `Io_Acked`. An empty ack (`nSize` 0) still owes a `Recv_Ack`.

## Ending

### Close

Socket.IO `disconnect`. The handle stays readable.

### Free

Retires the handle. Disconnects first if still up.

## Reading state

`State`, `Buffered`, `Url`, `Error`, `IsValid`, `Index` — same roles as on [`SOCKET`](SOCKET.md).

## Host contract

The engine is not in this repository. Implement against this page:

- Use official `socket.io-client`, one client per `twIoIx`.
- `IO_OPEN` → `io(sUrl)` with default transports. Empty URL or a dead fabric → `0`.
- `connect` → `IO_OPENED`. Emit refused until then.
- `socket.onAny` → enqueue `{sEvent, bBinary, aByte}` then `IO_RECEIVED`.
- `connect_error` → set `Error`, `IO_FAILED`, then `IO_CLOSED` with `wCode=1006`, `bClean=false`.
- `disconnect` → `IO_CLOSED` (`1000`/`true` if the guest called `Close`, else `1006`/`false`).
- `IO_EMIT_TEXT` / `IO_EMIT_BINARY` → `socket.emit(sEvent, payload)` while OPEN. No ack callback.
- `IO_EMIT_TEXT_EX` / `IO_EMIT_BINARY_EX` → `socket.emit(sEvent, payload, (ack) => { enqueue ack; Notify IO_ACKED })`. Stringify objects as JSON text; `Buffer`/`ArrayBuffer` as binary. `nSize` 0 is a valid empty ack.
- `IO_CLOSE` → `disconnect()`. Handle stays readable.
- `IO_FREE` → disconnect if still up, then drop the handle.
- Two bounded queues (events, acks). 16 MB payload cap.
- No ack timeout, rooms, auth, or namespaces in v1.

## See also

- [`NETWORK`](NETWORK.md) - `Io_Open`.
- [`INSTANCE`](INSTANCE.md) - the five `Io_*` hooks.
- [`SOCKET`](SOCKET.md) - the WebSocket analog this object sits beside.
- [API overview](overview.md).
