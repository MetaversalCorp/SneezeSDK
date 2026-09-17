# INSTANCE

The lifecycle trait your module implements. The engine drives a running WASM instance through four lifecycle callbacks plus six asynchronous event callbacks; you provide whichever you need and register the type with the [`instance!`](#the-instance-macro) macro, which generates the raw ABI exports and forwards to your implementation.

Every method has a default empty body, so you override only what you use.

The name mirrors the engine's own term for a loaded module: the engine calls it a `WASM_INSTANCE`. (A *declared* module in a fabric manifest is a separate thing, the [`MODULE`](MODULE.md) record.)

## Methods

### Init

```rust
fn Init () {}
```

- **Parameters:** none.
- **Returns:** nothing.
- **Description:** Called once, when the module is first loaded, before any fabric opens. Use it for one-time global setup that is not tied to a specific fabric. Most modules leave it as the default.
- **Example:**

```rust
impl INSTANCE for MY_MODULE
{
   fn Init ()
   {
      // one-time setup, no fabric yet
   }
}
```

- **See also:** [`Shutdown`](#shutdown) (its mirror), [`Open`](#open).

### Open

```rust
fn Open (pHost: &HOST) {}
```

- **Parameters:**
  - `pHost` - the [`HOST`](HOST.md) root handle for the fabric that just opened. Every subsystem *and* the immutable configuration is reached through it.
- **Returns:** nothing.
- **Description:** The main entry point. Called once per fabric that loads your module. This is where you read configuration, build the scene, and store or restore state. Because one instance can serve several fabrics at once, key any per-fabric state you retain by `pHost.Index ()`. The engine pushes an immutable [Open snapshot](SNAPSHOT.md) at this point; the SDK parses it privately, so you read it through the fabric's typed views (`pHost.Resource ()`, `pHost.Container ()`, ...) rather than touching any raw blob.
- **Example:**

```rust
fn Open (pHost: &HOST)
{
   pHost.Console ().Log (pHost.Container ().Name ());

   let mut pRoot = SNEEZE_ABI_MAPOBJECT::Physical ();
   pRoot.Name ("Stool").Reference ("assets/Stool.glb");
   pHost.Fabric ().Node_Root (&pRoot);
}
```

- **See also:** [`Close`](#close) (its mirror), [`HOST`](HOST.md), [the Open snapshot](SNAPSHOT.md).

### Close

```rust
fn Close (pHost: &HOST) {}
```

- **Parameters:**
  - `pHost` - the [`HOST`](HOST.md) handle for the fabric being closed (the same index you saw in `Open`).
- **Returns:** nothing.
- **Description:** Called when a fabric unloads. Release any per-fabric state you keyed by `pHost.Index ()`. The engine tears down the fabric's nodes itself; you only clean up your own guest-side bookkeeping.
- **Example:**

```rust
fn Close (pHost: &HOST)
{
   // drop any state keyed by pHost.Index ()
}
```

- **See also:** [`Open`](#open) (its mirror).

### Shutdown

```rust
fn Shutdown () {}
```

- **Parameters:** none.
- **Returns:** nothing.
- **Description:** Called once, when the module itself is unloading, after all fabrics have closed. The mirror of `Init`; use it for one-time global teardown.
- **Example:**

```rust
fn Shutdown ()
{
   // one-time teardown
}
```

- **See also:** [`Init`](#init) (its mirror).

### Timer

```rust
fn Timer (pHost: &HOST, twTimerIx: u64, qwParam: u64) {}
```

- **Parameters:**
  - `pHost` - the [`HOST`](HOST.md) that armed the timer.
  - `twTimerIx` - the id returned by [`TIMER::Set`](TIMER.md#set) or [`TIMER::Interval`](TIMER.md#interval) when the timer was armed.
  - `qwParam` - the opaque cookie you passed when arming; the engine echoes it back so one handler can tell its timers apart.
- **Returns:** nothing.
- **Description:** Called when a [`TIMER`](TIMER.md) you armed fires. Unlike the lifecycle methods, this is asynchronous - it arrives between `Open` and `Close` for the arming fabric, possibly many times for a repeating timer. Leave it as the default empty body if your module arms no timers.
- **Example:**

```rust
fn Timer (pHost: &HOST, twTimerIx: u64, qwParam: u64)
{
   pHost.Console ().Log (&format! ("timer {} fired (param {})", twTimerIx, qwParam));
}
```

- **See also:** [`TIMER`](TIMER.md).

### Request

```rust
fn Request (pHost: &HOST, pRequest: REQUEST, bSuccess: bool) {}
```

- **Parameters:**
  - `pHost` - the [`HOST`](HOST.md) whose fabric opened the request.
  - `pRequest` - a [`REQUEST`](REQUEST.md) handle naming the finished exchange. It wraps the same id as the handle you sent, so `pRequest.Index ()` matches.
  - `bSuccess` - `true` if the exchange completed, `false` if the transport failed. This is **not** the HTTP status: a `404` arrives with `bSuccess` `true` and `pRequest.Status ()` of `404`. Only a DNS failure, refused connection, timeout, failed integrity check, or oversized body sets it `false`, and then [`REQUEST::Error`](REQUEST.md#error) carries the reason.
- **Returns:** nothing.
- **Description:** Called when a [`REQUEST`](REQUEST.md) you sent finishes. Like [`Timer`](#timer) this is asynchronous, arriving between `Open` and `Close` for the fabric that opened it. Read the response through `pRequest`, then [`Close`](REQUEST.md#close) it - the guest owns the handle, so leaving it open keeps the response buffered host-side. Leave this as the default empty body if your module sends no requests.
- **Example:**

```rust
fn Request (pHost: &HOST, pRequest: REQUEST, bSuccess: bool)
{
   if bSuccess  &&  pRequest.Status () == 200
   {
      pHost.Console ().Log (&pRequest.Text ());
   }
   else
   {
      pHost.Console ().Error (&format! ("failed ({}): {}", pRequest.Status (), pRequest.Error ()));
   }

   pRequest.Close ();
}
```

- **See also:** [`REQUEST`](REQUEST.md), [`NETWORK`](NETWORK.md).

### Socket_Opened

```rust
fn Socket_Opened (pHost: &HOST, pSocket: SOCKET) {}
```

- **Parameters:**
  - `pHost` - the [`HOST`](HOST.md) whose fabric opened the socket.
  - `pSocket` - a [`SOCKET`](SOCKET.md) handle naming the connection. It wraps the same id as the handle you opened, so `pSocket.Index ()` matches.
- **Returns:** nothing.
- **Description:** Called when a socket's handshake succeeds. This is `WebSocket.onopen`, and it is the first moment you may send: a send before this arrives is refused. [`Protocol`](SOCKET.md#protocol) becomes meaningful here too.
- **Example:**

```rust
fn Socket_Opened (pHost: &HOST, pSocket: SOCKET)
{
   pSocket.Send_Text ("{\"join\":\"lobby\"}");
}
```

- **See also:** [`SOCKET`](SOCKET.md), [`NETWORK::Socket_Open`](NETWORK.md#socket_open).

### Socket_Received

```rust
fn Socket_Received (pHost: &HOST, pSocket: SOCKET, bBinary: bool, nSize: i64) {}
```

- **Parameters:**
  - `pHost`, `pSocket` - as [`Socket_Opened`](#socket_opened).
  - `bBinary` - `true` for a binary frame, `false` for a text frame.
  - `nSize` - the message's size in bytes.
- **Returns:** nothing.
- **Description:** Called when a message is waiting. This is `WebSocket.onmessage`, with one difference worth internalizing: it does **not** hand you the bytes. Take them with [`Recv`](SOCKET.md#recv) or [`Recv_Text`](SOCKET.md#recv_text). A message you do not take stays queued, and a module that stops taking them overflows the queue - see [the receive queue](SOCKET.md#the-receive-queue).
- **Example:**

```rust
fn Socket_Received (pHost: &HOST, pSocket: SOCKET, bBinary: bool, nSize: i64)
{
   if bBinary
   {
      let aByte = pSocket.Recv ();

      pHost.Console ().Log (&format! ("{} binary bytes", aByte.len ()));
   }
   else
   {
      pHost.Console ().Log (&pSocket.Recv_Text ());
   }
}
```

- **See also:** [`SOCKET::Recv`](SOCKET.md#recv).

### Socket_Failed

```rust
fn Socket_Failed (pHost: &HOST, pSocket: SOCKET) {}
```

- **Parameters:** as [`Socket_Opened`](#socket_opened).
- **Returns:** nothing.
- **Description:** Called when the socket fails - a refused connection, a failed handshake, a TLS failure, or a receive queue that overflowed. This is `WebSocket.onerror`. It is **always followed** by [`Socket_Closed`](#socket_closed) with code `1006`, because a failure means no closing handshake happened. Read [`Error`](SOCKET.md#error) here for the reason.
- **See also:** [`SOCKET::Error`](SOCKET.md#error).

### Socket_Closed

```rust
fn Socket_Closed (pHost: &HOST, pSocket: SOCKET, wCode: i32, bClean: bool) {}
```

- **Parameters:**
  - `pHost`, `pSocket` - as [`Socket_Opened`](#socket_opened).
  - `wCode` - the WebSocket close code. `1000` is a normal closure; `1006` means the connection died without a handshake, which is what a failure produces.
  - `bClean` - `true` if the closing handshake completed.
- **Returns:** nothing.
- **Description:** Called when the connection is finished, whichever side ended it. This is `WebSocket.onclose`. The handle is still readable here, so you can log the code or the URL - but it is not released, and this is the natural place to [`Free`](SOCKET.md#free) it.
- **Example:**

```rust
fn Socket_Closed (pHost: &HOST, pSocket: SOCKET, wCode: i32, bClean: bool)
{
   pHost.Console ().Log (&format! ("closed {} (clean: {})", wCode, bClean));

   pSocket.Free ();
}
```

- **See also:** [`SOCKET::Close`](SOCKET.md#close), [`SOCKET::Free`](SOCKET.md#free).

## The instance! macro

```rust
sneeze::instance! (MY_MODULE);
```

Place this once, at module scope, passing the type that implements `INSTANCE`. It generates the seven raw ABI exports the engine looks up by name - `Init`, `Open`, `Close`, `Shutdown`, `Alloc`, `Free`, `Notify` - and routes each to your implementation (or to the SDK's own memory management, for `Alloc`/`Free`). The generated `Notify` decodes each host event and dispatches it to the matching hook - a timer fire to [`Timer`](#timer), a finished request to [`Request`](#request), a socket event to one of the four `Socket_*` hooks; an event with no hook is ignored. You never write those exports yourself.

- **Description:** Without this macro the engine cannot find your module's entry points, because it resolves them as named WASM exports. The macro is the one required piece of boilerplate.
- **Example:**

```rust
use sneeze::*;

struct MY_MODULE;

impl INSTANCE for MY_MODULE
{
   fn Open (pHost: &HOST)
   {
      pHost.Console ().Log ("loaded");
   }
}

sneeze::instance! (MY_MODULE);
```

- **See also:** [Incorporating the ABI](../incorporating-the-abi.md) (the raw exports the macro emits), [API overview](overview.md).
