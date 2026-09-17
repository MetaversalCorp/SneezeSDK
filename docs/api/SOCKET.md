# SOCKET

One WebSocket connection, opened from [`NETWORK`](NETWORK.md). It is modeled directly on the browser's `WebSocket`: you open it, wait for it, send and receive frames, and close it. A `SOCKET` is a thin copyable handle (just an id), so passing it around costs nothing and every copy names the same connection.

Where a [`REQUEST`](REQUEST.md) is one question and one answer, a socket is a conversation. That is the whole difference in how you use it: instead of one completion callback there are four event callbacks, and instead of a response you read a queue.

## The shape of a socket

```rust
impl INSTANCE for MY_MODULE
{
   fn Open (pHost: &HOST)
   {
      let pSocket = pHost.Network ().Socket_Open ("wss://example.com/chat");

      // Nothing can be sent yet - the handshake has not finished.
      let _ = pSocket;
   }

   fn Socket_Opened (pHost: &HOST, pSocket: SOCKET)
   {
      pSocket.Send_Text ("{\"join\":\"lobby\"}");
   }

   fn Socket_Received (pHost: &HOST, pSocket: SOCKET, bBinary: bool, nSize: i64)
   {
      pHost.Console ().Log (&pSocket.Recv_Text ());
   }

   fn Socket_Failed (pHost: &HOST, pSocket: SOCKET)
   {
      pHost.Console ().Error (&pSocket.Error ());
   }

   fn Socket_Closed (pHost: &HOST, pSocket: SOCKET, wCode: i32, bClean: bool)
   {
      pSocket.Free ();
   }
}
```

Three rules matter, and they are the three ways a socket differs from a request:

1. **You cannot send until it opens.** Sending before [`Socket_Opened`](INSTANCE.md#socket_opened) is refused, the same way the browser throws `InvalidStateError`.
2. **Every `Socket_Received` owes a `Recv`.** The callback tells you a message is waiting and how big it is; it does not hand you the bytes. A message you never take stays queued, and a module that stops taking them eventually overflows the queue - see [the receive queue](#the-receive-queue).
3. **`Close` and `Free` are different calls.** `Close` ends the *connection*; `Free` ends the *handle*. See [two ways to end a socket](#two-ways-to-end-a-socket).

## State

[`State`](#state) reports where the connection stands. The values mirror `WebSocket.readyState` exactly, including their numbers:

| State | Meaning |
|-------|---------|
| `kSNEEZE_ABI_SOCKET_STATE_CONNECTING` | the handshake is in progress |
| `kSNEEZE_ABI_SOCKET_STATE_OPEN` | sending and receiving are live |
| `kSNEEZE_ABI_SOCKET_STATE_CLOSING` | the closing handshake is in progress |
| `kSNEEZE_ABI_SOCKET_STATE_CLOSED` | the connection is finished |

A failure is not a fifth state. [`Socket_Failed`](INSTANCE.md#socket_failed) is **always followed** by [`Socket_Closed`](INSTANCE.md#socket_closed) with code `1006`, because a failure means the connection died without a closing handshake and `1006` is the code the protocol reserves for exactly that. So a socket that fails still ends up `CLOSED`, and [`Error`](#error) is what distinguishes it from a clean close.

## The receive queue

An incoming message is queued for you rather than pushed at you, because the callback arrives on the engine's network thread and your module runs on its own. [`Socket_Received`](INSTANCE.md#socket_received) is the notification; [`Recv`](#recv) is the collection.

[`Recv`](#recv) takes the head of the queue only when it has somewhere to fit, which is why the SDK asks the host for the size first and then reads it exactly. That is not merely an optimization: it is what keeps a message that would not fit from being consumed and lost.

The queue is **bounded**. A module that stops calling `Recv` while the server keeps talking eventually fills it, and from that point messages are **dropped** and [`Error`](#error) says so. This is deliberate - the alternative is letting a chatty server grow the host's memory without limit.

## Two ways to end a socket

They are not interchangeable, and the difference is the same one the browser draws:

- [`Close`](#close) runs the **closing handshake**. The connection winds down, `Socket_Closed` arrives, and the handle stays readable - so you can still ask what the close code was, or what the URL had been. This is `WebSocket.close()`.
- [`Free`](#free) retires the **handle**. It is the mirror of [`NETWORK::Socket_Open`](NETWORK.md#socket_open), and it is yours to call. Until you do, the host holds the connection for the fabric's life. The browser has no equivalent because a JavaScript `WebSocket` is garbage-collected; a WASM guest has no collector, so the handle is explicit.

`Free` on a socket that is still up closes it first, so you never have to sequence the two yourself. The usual place to call it is inside `Socket_Closed`.

## Sending

### Send_Text

```rust
pub fn Send_Text (&self, sText: &str) -> bool
```

- **Parameters:**
  - `sText` - the message as UTF-8 text.
- **Returns:** `true` if the engine accepted the frame for transmission. This says nothing about it arriving.
- **Description:** Sends one text frame. Refused unless the socket is `OPEN`, and refused if the frame exceeds the 16 MB frame cap.
- **See also:** [`Send_Bytes`](#send_bytes), [`Buffered`](#buffered).

### Send_Bytes

```rust
pub fn Send_Bytes (&self, aByte: &[u8]) -> bool
```

- **Parameters:**
  - `aByte` - the message as raw bytes.
- **Returns:** as [`Send_Text`](#send_text).
- **Description:** Sends one binary frame. The receiving side sees `bBinary` `true` on its message event. Same `OPEN`-only and frame-cap rules as [`Send_Text`](#send_text).
- **See also:** [`Send_Text`](#send_text).

## Receiving

### Recv

```rust
pub fn Recv (&self) -> Vec<u8>
```

- **Returns:** the message at the head of the queue, or an empty result when nothing is waiting.
- **Description:** Takes one message. Call it once per [`Socket_Received`](INSTANCE.md#socket_received); calling it when the queue is empty is a harmless no-op. A binary and a text message are both bytes here - the callback's `bBinary` is what tells you which you are looking at.
- **See also:** [`Recv_Text`](#recv_text), [the receive queue](#the-receive-queue).

### Recv_Text

```rust
pub fn Recv_Text (&self) -> String
```

- **Returns:** the message at the head of the queue as text; empty when nothing is waiting, and empty if the bytes are not valid UTF-8.
- **Description:** [`Recv`](#recv) for the common case. Note that the message is consumed either way, so an invalid-UTF-8 message read this way is gone - use `Recv` if you need to see the bytes.
- **See also:** [`Recv`](#recv).

## Ending it

### Close

```rust
pub fn Close (&self) -> bool
```

- **Parameters:** none.
- **Returns:** `true` if the engine accepted the close.
- **Description:** Runs the closing handshake with code `1000` (a normal closure) and no reason. [`Socket_Closed`](INSTANCE.md#socket_closed) still arrives, so you learn the close completed the same way you would if the server had initiated it.
- **See also:** [`Close_Ex`](#close_ex), [`Free`](#free).

### Close_Ex

```rust
pub fn Close_Ex (&self, wCode: i32, sReason: &str) -> bool
```

- **Parameters:**
  - `wCode` - the WebSocket close code. `1000` is a normal closure; `1001` means going away.
  - `sReason` - a human-readable reason. The protocol caps this at 123 bytes and a longer one is truncated (on a character boundary, so it stays valid UTF-8).
- **Returns:** as [`Close`](#close).
- **Description:** [`Close`](#close) with a specific code and reason, which the peer receives on its own close event.
- **See also:** [`Close`](#close).

### Free

```rust
pub fn Free (&self) -> bool
```

- **Parameters:** none.
- **Returns:** `true` if the handle was retired.
- **Description:** Releases the handle. The mirror of [`NETWORK::Socket_Open`](NETWORK.md#socket_open) and **not** optional: until you call it the host holds the connection for the fabric's life. Closes the connection first if it is still up. Every method on a freed handle is a harmless no-op.
- **See also:** [two ways to end a socket](#two-ways-to-end-a-socket).

## Reading state

### State

```rust
pub fn State (&self) -> eSNEEZE_ABI_SOCKET_STATE
```

- **Returns:** one of the four values in [State](#state) above.
- **Description:** Where the connection stands. A handle the host no longer knows reads as `CLOSED`, which is the safe answer.

### Buffered

```rust
pub fn Buffered (&self) -> i64
```

- **Returns:** the number of bytes handed to the socket that have not reached the wire yet.
- **Description:** `WebSocket.bufferedAmount`. Useful for backpressure: a number that keeps growing across sends means you are producing faster than the connection can drain, and should slow down. A send does not drain synchronously, so this is usually non-zero for a moment right after one.

### Url

```rust
pub fn Url (&self) -> String
```

- **Returns:** the socket's URL.
- **Description:** The URL the socket was opened with. Known immediately, unlike [`Protocol`](#protocol).

### Protocol

```rust
pub fn Protocol (&self) -> String
```

- **Returns:** the negotiated subprotocol; empty if none.
- **Description:** Which of the subprotocols you offered to [`Socket_Open_Ex`](NETWORK.md#socket_open_ex) the server chose. Empty until the socket opens, and empty after that if the server chose none or you offered none.

### Error

```rust
pub fn Error (&self) -> String
```

- **Returns:** the error text; **empty unless something failed**.
- **Description:** Why the socket failed - a refused connection, a failed handshake, a TLS failure, or a receive queue that overflowed. A clean close is not an error, so a socket you closed yourself leaves this empty. Read it on [`Socket_Failed`](INSTANCE.md#socket_failed).

### IsValid

```rust
pub fn IsValid (&self) -> bool
```

- **Returns:** `true` unless the opening call failed.
- **Description:** Whether the handle names a real connection. Check it after [`NETWORK::Socket_Open`](NETWORK.md#socket_open); every other method on an invalid handle is a harmless no-op.

### Index

```rust
pub fn Index (&self) -> u64
```

- **Returns:** the socket's handle id (`0` if invalid).
- **Description:** The raw id, if you want to key your own per-socket state by it. The `SOCKET` you receive on a callback is a fresh handle wrapping the same id, so comparing `Index ()` is how you match a callback to the socket you opened.

## See also

- [`NETWORK`](NETWORK.md) - the view that opens sockets, and why a socket URL is not resolved against the fabric.
- [`INSTANCE::Socket_Opened`](INSTANCE.md#socket_opened) - and the three sibling callbacks a socket delivers.
- [`REQUEST`](REQUEST.md) - the other half of `NETWORK`, for one-shot HTTP.
- [API overview](overview.md).
