# NETWORK

The fabric's window onto the network, reached through [`HOST::Network`](HOST.md#network). It is a zero-cost view over the fabric handle, and its job is to open two kinds of object: [`REQUEST`](REQUEST.md), one per HTTP exchange, shaped like the browser's `XMLHttpRequest`; and [`SOCKET`](SOCKET.md), one per WebSocket connection, shaped like the browser's `WebSocket`.

`NETWORK` itself has no state and no other behaviour. Everything you do with an exchange or a connection happens on the handle it gives you back.

## URLs are relative to the fabric

A URL is resolved **host-side against the fabric's own URL**, using the same rules as a node's resource reference or a module reference in the manifest. So for a fabric published at `https://cdn.example.com/worlds/demo/demo.msf.json`:

| You ask for | The engine fetches |
|-------------|--------------------|
| `api/state` | `https://cdn.example.com/worlds/demo/api/state` |
| `../shared/api` | `https://cdn.example.com/worlds/shared/api` |
| `/api/state` | `https://cdn.example.com/api/state` |
| `https://other.example.com/api` | `https://other.example.com/api` (used verbatim) |

This means a fabric can be moved or mirrored without editing the URLs inside its module.

## Caching follows the verb

A `GET` goes through the engine's ordinary asset cache, so a repeated `GET` of an unchanged resource is answered from disk without touching the network ([`REQUEST::IsCached`](REQUEST.md#iscached) tells you which happened). **Every other verb bypasses the cache**, exactly as a browser treats a non-`GET` as non-cacheable. Non-`GET` responses are still written to the session's transitory storage so they remain visible in the browser's network inspector, and are discarded when the session ends.

## Methods

### Request_Open

```rust
pub fn Request_Open (&self, eVerb: eSNEEZE_ABI_REQUEST_VERB, sUrl: &str) -> REQUEST
```

- **Parameters:**
  - `eVerb` - the HTTP verb (see the table below).
  - `sUrl` - the URL, resolved against the fabric's own URL as described above.
- **Returns:** a [`REQUEST`](REQUEST.md) handle. Check [`IsValid`](REQUEST.md#isvalid): it is invalid if the host refused the open, which happens when the URL is empty or the fabric is no longer live.
- **Description:** Opens one HTTP exchange without sending it, so you can set headers and a timeout first. Nothing touches the network until you call [`Send`](REQUEST.md#send). The mirror of this call is [`REQUEST::Close`](REQUEST.md#close), which you own.
- **Example:**

```rust
fn Open (pHost: &HOST)
{
   let pRequest = pHost.Network ().Request_Open (eSNEEZE_ABI_REQUEST_VERB::kSNEEZE_ABI_REQUEST_VERB_GET, "api/state");

   pRequest.Send ();
}
```

- **See also:** [`Request_Open_Ex`](#request_open_ex), [`REQUEST::Send`](REQUEST.md#send), [`REQUEST::Close`](REQUEST.md#close).

### Request_Open_Ex

```rust
pub fn Request_Open_Ex (&self, eVerb: eSNEEZE_ABI_REQUEST_VERB, sUrl: &str, sIntegrity: &str) -> REQUEST
```

- **Parameters:**
  - `eVerb`, `sUrl` - as [`Request_Open`](#request_open).
  - `sIntegrity` - a subresource-integrity hash of the form `sha256-<lowercase hex>`. `sha384-` and `sha512-` are also accepted. An empty string means no check, making this identical to `Request_Open`.
- **Returns:** as [`Request_Open`](#request_open).
- **Description:** As [`Request_Open`](#request_open), but pins the response to a known hash. The engine hashes the bytes it received and **fails the request** if they do not match, so a tampered or truncated response never reaches your module. Use this for content you expect to be immutable.
- **Example:**

```rust
let pRequest = pHost.Network ().Request_Open_Ex (
   eSNEEZE_ABI_REQUEST_VERB::kSNEEZE_ABI_REQUEST_VERB_GET,
   "assets/table.json",
   "sha256-96d4c0b6f98adb9819e1f242aa6664d7bd825025dc0159b1ca4420ea08e6c857");

pRequest.Send ();
```

- **See also:** [`Request_Open`](#request_open).

### Socket_Open

```rust
pub fn Socket_Open (&self, sUrl: &str) -> SOCKET
```

- **Parameters:**
  - `sUrl` - an **absolute** `ws://` or `wss://` URL.
- **Returns:** a [`SOCKET`](SOCKET.md) handle. Check [`IsValid`](SOCKET.md#isvalid): it is invalid if the host refused the open, which happens when the URL is not a WebSocket URL or the fabric is no longer live.
- **Description:** Opens one WebSocket connection. The handshake is asynchronous, so the socket comes back `CONNECTING` and you wait for [`INSTANCE::Socket_Opened`](INSTANCE.md#socket_opened) before sending anything. The mirror of this call is [`SOCKET::Free`](SOCKET.md#free), which you own - note that [`Close`](SOCKET.md#close) ends the *connection*, not the handle.
- **Example:**

```rust
fn Open (pHost: &HOST)
{
   let pSocket = pHost.Network ().Socket_Open ("wss://example.com/chat");

   // Nothing to send yet - wait for Socket_Opened.
   let _ = pSocket;
}
```

- **See also:** [`Socket_Open_Ex`](#socket_open_ex), [`SOCKET`](SOCKET.md), [`INSTANCE::Socket_Opened`](INSTANCE.md#socket_opened).

### Socket_Open_Ex

```rust
pub fn Socket_Open_Ex (&self, sUrl: &str, sProtocol: &str) -> SOCKET
```

- **Parameters:**
  - `sUrl` - as [`Socket_Open`](#socket_open).
  - `sProtocol` - the subprotocols to offer, comma-separated in order of preference. An empty string offers none, making this identical to `Socket_Open`.
- **Returns:** as [`Socket_Open`](#socket_open).
- **Description:** As [`Socket_Open`](#socket_open), but offers subprotocols during the handshake. The server picks at most one; read which with [`SOCKET::Protocol`](SOCKET.md#protocol) once the socket opens. It stays empty if the server chose none.
- **Example:**

```rust
let pSocket = pHost.Network ().Socket_Open_Ex ("wss://example.com/chat", "chat.v2,chat.v1");
```

- **See also:** [`Socket_Open`](#socket_open), [`SOCKET::Protocol`](SOCKET.md#protocol).

## Socket URLs are *not* relative

This is the one place the two halves of `NETWORK` deliberately disagree. A request URL is resolved against the fabric (above); a socket URL is **not** - it must be absolute, with a `ws://` or `wss://` scheme. That is the rule the browser's `WebSocket` constructor applies, and following it means a socket URL means the same thing in a Sneeze module as it does on the web.

## The verb

The verb is an [`eSNEEZE_ABI_REQUEST_VERB`](../incorporating-the-abi.md):

| Verb | Cached | Body allowed |
|------|--------|--------------|
| `kSNEEZE_ABI_REQUEST_VERB_GET` | yes | no |
| `kSNEEZE_ABI_REQUEST_VERB_POST` | no | yes |
| `kSNEEZE_ABI_REQUEST_VERB_PUT` | no | yes |
| `kSNEEZE_ABI_REQUEST_VERB_PATCH` | no | yes |
| `kSNEEZE_ABI_REQUEST_VERB_DELETE` | no | yes |
| `kSNEEZE_ABI_REQUEST_VERB_HEAD` | no | no |

`GET` is `0` because it is the default and the only cached verb. A body sent with `GET` or `HEAD` is ignored.

## Limits

A single guest request is capped at **256 MB** of response body. A response that exceeds the cap fails as a transport error rather than growing your module's memory without bound; [`REQUEST::Error`](REQUEST.md#error) says so.

A single WebSocket frame is capped at **16 MB** in either direction. A conversation that needs more than that per message wants a request, not a socket.

## See also

- [`REQUEST`](REQUEST.md) - the exchange this view opens, and every method for reading the answer.
- [`SOCKET`](SOCKET.md) - the connection this view opens, and every method for driving it.
- [`INSTANCE::Request`](INSTANCE.md#request) - the callback a finished exchange is delivered to.
- [`INSTANCE::Socket_Opened`](INSTANCE.md#socket_opened) - and the three sibling socket callbacks.
- [`HOST::Network`](HOST.md#network) - where this view comes from.
- [API overview](overview.md).
