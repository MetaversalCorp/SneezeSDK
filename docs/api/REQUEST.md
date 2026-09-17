# REQUEST

One HTTP exchange, opened from [`NETWORK`](NETWORK.md). It is modeled directly on the browser's `XMLHttpRequest`: you open it, configure it, send it, and read the answer from a callback. A `REQUEST` is a thin copyable handle (just an id), so passing it around costs nothing and every copy names the same exchange.

The answer does **not** come back from `Send`. It arrives later as an [`INSTANCE::Request`](INSTANCE.md#request) callback, at which point the response getters below become meaningful.

## The shape of a request

```rust
impl INSTANCE for MY_MODULE
{
   fn Open (pHost: &HOST)
   {
      let pRequest = pHost.Network ().Request_Open (eSNEEZE_ABI_REQUEST_VERB::kSNEEZE_ABI_REQUEST_VERB_POST, "api/join");

      pRequest.Header_Set ("Content-Type", "application/json");
      pRequest.Timeout_Set (5000);
      pRequest.Send_Text ("{\"name\":\"demo\"}");
   }

   fn Request (pHost: &HOST, pRequest: REQUEST, bSuccess: bool)
   {
      if bSuccess
      {
         pHost.Console ().Log (&format! ("{} -> {}", pRequest.Status (), pRequest.Text ()));
      }
      else
      {
         pHost.Console ().Error (&pRequest.Error ());
      }

      pRequest.Close ();
   }
}
```

Three rules matter:

1. **Configure before you send.** `Header_Set` and `Timeout_Set` are ignored once the request is in flight.
2. **Send once per handle.** A second `Send` on the same `REQUEST` is refused.
3. **You own the handle.** `Close` is the mirror of `Request_Open`. Until you call it the host keeps the whole response buffered for the fabric's life, so a module that opens requests and never closes them leaks host memory.

## Success is not the HTTP status

`bSuccess` on the callback answers "did the exchange complete?", not "did the server like it?". A `404` or a `500` is a **successful** exchange: `bSuccess` is `true`, [`Status`](#status) is `404`, [`Error`](#error) is empty, and the response body is intact so you can read the server's error document. `bSuccess` is `false` only for a transport failure - DNS failure, connection refused, timeout, a failed integrity check, or a response over the size cap - and then [`Error`](#error) carries the reason.

This mirrors `XMLHttpRequest`, where `onload` fires for a 404 and `onerror` fires only for a network-level failure.

## State

[`State`](#state) reports where the exchange stands, the analog of XHR's `readyState`:

| State | Meaning |
|-------|---------|
| `kSNEEZE_ABI_REQUEST_STATE_IDLE` | opened, not yet sent |
| `kSNEEZE_ABI_REQUEST_STATE_SENDING` | in flight |
| `kSNEEZE_ABI_REQUEST_STATE_COMPLETE` | the server answered (at any status) |
| `kSNEEZE_ABI_REQUEST_STATE_FAILED` | the transport failed |
| `kSNEEZE_ABI_REQUEST_STATE_ABORTED` | you called [`Abort`](#abort) |

## Configuration

### Header_Set

```rust
pub fn Header_Set (&self, sName: &str, sValue: &str) -> bool
```

- **Parameters:**
  - `sName` - the header name, e.g. `"Content-Type"`.
  - `sValue` - the header value.
- **Returns:** `true` if the header was recorded; `false` if the handle is unknown or already sent.
- **Description:** Sets one request header. Call it as many times as you have headers. Must precede [`Send`](#send).
- **See also:** [`Header`](#header), [`Header_All`](#header_all).

### Timeout_Set

```rust
pub fn Timeout_Set (&self, nMilli: i32) -> bool
```

- **Parameters:**
  - `nMilli` - the timeout in milliseconds.
- **Returns:** `true` if the timeout was recorded; `false` if the handle is unknown or already sent.
- **Description:** Caps how long this exchange may take. On expiry the request fails as a transport error, so it arrives with `bSuccess` `false` and a timeout message in [`Error`](#error). Must precede [`Send`](#send).

## Sending

### Send

```rust
pub fn Send (&self) -> bool
```

- **Parameters:** none.
- **Returns:** `true` if the engine accepted the request for dispatch. This says nothing about the outcome - that arrives at [`INSTANCE::Request`](INSTANCE.md#request).
- **Description:** Issues the request with no body. The natural form for `GET`, `HEAD`, and `DELETE`.
- **See also:** [`Send_Text`](#send_text), [`Send_Bytes`](#send_bytes).

### Send_Text

```rust
pub fn Send_Text (&self, sText: &str) -> bool
```

- **Parameters:**
  - `sText` - the request body as UTF-8 text.
- **Returns:** as [`Send`](#send).
- **Description:** Issues the request with a text body - typically JSON, paired with a `Content-Type` header you set yourself (the engine does not guess one). The body is ignored on a `GET` or `HEAD`.
- **See also:** [`Send`](#send), [`Send_Bytes`](#send_bytes).

### Send_Bytes

```rust
pub fn Send_Bytes (&self, aByte: &[u8]) -> bool
```

- **Parameters:**
  - `aByte` - the request body as raw bytes.
- **Returns:** as [`Send`](#send).
- **Description:** Issues the request with a binary body. The body is ignored on a `GET` or `HEAD`.
- **See also:** [`Send`](#send), [`Send_Text`](#send_text).

## Ending

### Abort

```rust
pub fn Abort (&self) -> bool
```

- **Parameters:** none.
- **Returns:** `true` if the handle was found.
- **Description:** Stops the engine from delivering this exchange to your module: no [`INSTANCE::Request`](INSTANCE.md#request) callback will arrive, and [`State`](#state) becomes `ABORTED`. The handle stays alive, so you can still read it and you must still [`Close`](#close) it. Mirrors `XMLHttpRequest.abort`.
- **See also:** [`Close`](#close).

### Close

```rust
pub fn Close (&self) -> bool
```

- **Parameters:** none.
- **Returns:** `true` if the handle was found and released.
- **Description:** Releases the handle and discards the buffered response. The mirror of [`NETWORK::Request_Open`](NETWORK.md#request_open) and **not optional** - see rule 3 above. After `Close` every getter reads empty or zero. Any handle still open when the fabric closes is cleaned up then, but a long-lived fabric that never closes its requests will accumulate them.
- **See also:** [`Abort`](#abort), [`NETWORK::Request_Open`](NETWORK.md#request_open).

## Reading the answer

Every getter below is meaningful once [`INSTANCE::Request`](INSTANCE.md#request) has fired. Before that, [`State`](#state) reports `IDLE` or `SENDING` and the rest read empty or zero.

### State

```rust
pub fn State (&self) -> eSNEEZE_ABI_REQUEST_STATE
```

- **Returns:** the exchange's state, per the table above.
- **Description:** Where the request stands. Useful for polling from a timer if you would rather not rely on the callback, and for telling an abort apart from a failure after the fact.

### Status

```rust
pub fn Status (&self) -> i32
```

- **Returns:** the HTTP status code, or `0` if the server never answered.
- **Description:** The status line's code - `200`, `404`, `500`. Read it on every successful exchange; success does not imply `2xx`.
- **See also:** [`Status_Text`](#status_text).

### Status_Text

```rust
pub fn Status_Text (&self) -> String
```

- **Returns:** the reason phrase matching [`Status`](#status), e.g. `"Not Found"`; empty if there is no status.
- **Description:** The human-readable form of the status, for logging.

### Url

```rust
pub fn Url (&self) -> String
```

- **Returns:** the final absolute URL, after any redirects the engine followed.
- **Description:** Where the response actually came from. This is the resolved absolute URL, so it also shows you what your relative URL turned into.

### Header

```rust
pub fn Header (&self, sName: &str) -> String
```

- **Parameters:**
  - `sName` - the response header name, matched case-insensitively.
- **Returns:** the header's value, or an empty string if the response has no such header.
- **Description:** Reads one response header. An absent header and a present-but-empty one both read as empty; use [`Header_All`](#header_all) if you need to tell them apart.
- **See also:** [`Header_All`](#header_all), [`Content_Type`](#content_type).

### Header_All

```rust
pub fn Header_All (&self) -> String
```

- **Returns:** every response header as CRLF-separated `Name: value` lines.
- **Description:** The whole response header block in one string, matching `XMLHttpRequest.getAllResponseHeaders`. Parse it yourself if you need to enumerate headers.
- **See also:** [`Header`](#header).

### Body

```rust
pub fn Body (&self) -> Vec<u8>
```

- **Returns:** the response body as raw bytes; empty if there is no body.
- **Description:** The bytes exactly as received. Use this for binary payloads; use [`Text`](#text) for text. The SDK asks the host for the body's size and then reads it in one pass, so a large body costs two boundary crossings and one copy.
- **See also:** [`Text`](#text), [`Size`](#size).

### Text

```rust
pub fn Text (&self) -> String
```

- **Returns:** the response body decoded as UTF-8 text; empty if there is no body or the bytes are not valid UTF-8.
- **Description:** The convenient form for JSON and text responses, matching `XMLHttpRequest.responseText`. Parse JSON with your own compiled-in parser - the SDK does not impose one.
- **See also:** [`Body`](#body).

### Size

```rust
pub fn Size (&self) -> i64
```

- **Returns:** the response byte count.
- **Description:** How big the response was, without copying it. Useful for logging, or to decide whether you want the body at all.

### Content_Type

```rust
pub fn Content_Type (&self) -> String
```

- **Returns:** the response's content type, or empty if the server sent none.
- **Description:** A shortcut for `Header ("Content-Type")`, since it is the header you almost always want.
- **See also:** [`Header`](#header).

### Error

```rust
pub fn Error (&self) -> String
```

- **Returns:** the transport error text; **empty on success**.
- **Description:** Why the exchange failed at the transport level - a DNS failure, a refused connection, a timeout, a failed integrity check, or a body over the size cap. An HTTP error status is not a transport error, so a `404` leaves this empty. Read it when `bSuccess` is `false`.

### IsCached

```rust
pub fn IsCached (&self) -> bool
```

- **Returns:** `true` if the engine answered from its cache rather than the network.
- **Description:** Whether this exchange actually went out. Only a `GET` is ever cached, so this is always `false` for other verbs. See [caching follows the verb](NETWORK.md#caching-follows-the-verb).

### IsValid

```rust
pub fn IsValid (&self) -> bool
```

- **Returns:** `true` unless the opening call failed.
- **Description:** Whether the handle names a real exchange. Check it after [`NETWORK::Request_Open`](NETWORK.md#request_open); every other method on an invalid handle is a harmless no-op.

### Index

```rust
pub fn Index (&self) -> u64
```

- **Returns:** the request's handle id (`0` if invalid).
- **Description:** The raw id, if you want to key your own per-request state by it. The `REQUEST` you receive on the callback is a fresh handle wrapping the same id, so comparing `Index ()` is how you match a callback to the request you opened.

## See also

- [`NETWORK`](NETWORK.md) - the view that opens requests, URL resolution, caching, and limits.
- [`INSTANCE::Request`](INSTANCE.md#request) - the callback a finished exchange is delivered to.
- [`TIMER`](TIMER.md) - the other asynchronous callback the engine delivers.
- [API overview](overview.md).
