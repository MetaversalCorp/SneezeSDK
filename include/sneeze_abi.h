// Copyright 2026 Metaversal Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// ===========================================================================
// sneeze_abi.h - the canonical Sneeze WASM ABI contract.
//
// This header is the single source of truth for the wire format shared by the
// Sneeze engine (host) and every guest module, in every language. The host
// includes it to route calls; the Rust SDK mirrors it (see sdk/rust); a future
// C SDK layers ergonomic wrappers on top of it.
//
// (Named sneeze_abi.h, not sneeze.h, so it never collides with the engine's
// public include/Sneeze.h on case-insensitive filesystems.)
//
// The ABI has exactly two crossover functions and a tiny fixed set of guest
// exports, so a module compiled once keeps loading for years as the engine
// evolves (new methods are new numbers, never new symbols):
//
//   Import  (module "Sneeze"):
//     Call    (i32 nOffset, i32 nSize) -> i64               guest -> host request
//
//   Exports (the guest provides):
//     Alloc   (i32 nSize) -> i32 nOffset                    host writes into guest memory here
//     Free    (i32 nOffset, i32 nSize)                      release an Alloc block
//     Notify  (i32 nOffset, i32 nSize) -> i64               host -> guest event (events land later)
//     Init    ()                                            module loaded
//     Open    (i64 twFabricIx, i32 nOffset, i32 nSize)      a fabric opened (snapshot at nOffset)
//     Close   (i64 twFabricIx)                              a fabric closed
//     Shutdown()                                            module unloading
//
// Everything non-trivial is packed into a self-describing packet in the guest's
// own linear memory and routed by a (wType, wMethod) id, rather than resolved by
// one WASM symbol per call.
// ===========================================================================

#ifndef SNEEZE_ABI_H
#define SNEEZE_ABI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Version. Bumped only for a breaking change to the packet framing itself
// (never for adding a method - that is just a new wMethod number).
// ---------------------------------------------------------------------------

#define SNEEZE_ABI_VERSION      1

// ---------------------------------------------------------------------------
// Handles. HNODE is a 64-bit value handle (an object index - not a pointer,
// since it exceeds a wasm32 pointer). HREQUEST, HSOCKET and HIO are likewise
// 64-bit value handles - the twRequestIx that NETWORK's REQUEST_OPEN returns,
// the twSocketIx that SOCKET_OPEN returns, and the twIoIx that IO_OPEN returns,
// by which every other method of each is addressed. HMAPOBJECT is an opaque
// pointer to a guest-local builder. The fabric itself is reached through a
// guest-SDK HOST handle (see the language SDKs), which carries the u64 fabric
// index the wire formats below call twFabricIx. C gives these no type safety
// (bare integers); the Rust SDK wraps them in newtypes.
// ---------------------------------------------------------------------------

typedef uint64_t   HNODE;
typedef uint64_t   HREQUEST;
typedef uint64_t   HSOCKET;
typedef uint64_t   HIO;
typedef uint32_t   HMAPOBJECT;

// ---------------------------------------------------------------------------
// Packet header (8 bytes, little-endian). Every Call/Notify buffer begins with
// this, followed by dwSize bytes of method-specific payload.
// ---------------------------------------------------------------------------

typedef struct tagSNEEZE_ABI_PACKET_HEADER
{
   uint16_t                                                 wType;               // subsystem id (kSNEEZE_ABI_TYPE_*)
   uint16_t                                                 wMethod;             // method id within the subsystem
   uint32_t                                                 dwSize;              // payload byte count following the header
}
SNEEZE_ABI_PACKET_HEADER, *PSNEEZE_ABI_PACKET_HEADER;

// ---------------------------------------------------------------------------
// wType - subsystem registry. Fixed order; numbers are permanent.
// ---------------------------------------------------------------------------

enum eSNEEZE_ABI_TYPE
{
   kSNEEZE_ABI_TYPE_DATA                                 =  1,
   kSNEEZE_ABI_TYPE_CONSOLE                              =  2,
   kSNEEZE_ABI_TYPE_STORAGE                              =  3,
   kSNEEZE_ABI_TYPE_NETWORK                              =  4,
   kSNEEZE_ABI_TYPE_VIEWPORT                             =  5,
   kSNEEZE_ABI_TYPE_SCENE                                =  6,
   kSNEEZE_ABI_TYPE_FABRIC                               =  7,
   kSNEEZE_ABI_TYPE_NODE                                 =  8,
   kSNEEZE_ABI_TYPE_CHRONO                               =  9,
   kSNEEZE_ABI_TYPE_PERFORMANCE                          = 10,
   kSNEEZE_ABI_TYPE_TIMER                                = 11,
   kSNEEZE_ABI_TYPE_SERVICES                             = 12,
};

// ---------------------------------------------------------------------------
// wMethod - method registry, one enum per subsystem. Numbers are PERMANENT,
// MONOTONIC, and APPEND-ONLY: a revised method takes the next free number and
// never reuses a RETIRED one. This is the backward-compat guarantee - an old
// module keeps sending old numbers forever.
//
// A RESERVED RANGE is the single exception, and it is not a contradiction. A
// block declared reserved up front (see NETWORK) was never assigned to
// anything, so no module ever sent those numbers and filling them later cannot
// collide with anything deployed. The distinction that matters is:
//
//   reserved - never assigned, never sent, free to fill by design
//   retired  - assigned and shipped, then withdrawn, dead forever
//
// Only retired numbers are untouchable. A method still carrying the "not
// implemented yet" marker below has never been sent by anything and is
// therefore reserved, not retired.
// ---------------------------------------------------------------------------

// DATA is the fabric's config "Data" tree, served read-only (no Set/Remove) -
// the immutable analog of STORAGE. Path addressing is identical to STORAGE, but
// there is no scope (the data belongs to the one fabric).
enum eSNEEZE_ABI_METHOD_DATA
{
   kSNEEZE_ABI_METHOD_DATA_HAS                           =  1,
   kSNEEZE_ABI_METHOD_DATA_GET                           =  2,
};

enum eSNEEZE_ABI_METHOD_CONSOLE
{
   kSNEEZE_ABI_METHOD_CONSOLE_LOG                        =  1,
   kSNEEZE_ABI_METHOD_CONSOLE_DEBUG                      =  2,
   kSNEEZE_ABI_METHOD_CONSOLE_INFO                       =  3,
   kSNEEZE_ABI_METHOD_CONSOLE_WARN                       =  4,
   kSNEEZE_ABI_METHOD_CONSOLE_ERROR                      =  5,
   kSNEEZE_ABI_METHOD_CONSOLE_ASSERT                     =  6,
   kSNEEZE_ABI_METHOD_CONSOLE_GROUP                      =  7,
   kSNEEZE_ABI_METHOD_CONSOLE_GROUP_COLLAPSED            =  8,
   kSNEEZE_ABI_METHOD_CONSOLE_GROUP_END                  =  9,
   kSNEEZE_ABI_METHOD_CONSOLE_COUNT                      = 10,
   kSNEEZE_ABI_METHOD_CONSOLE_COUNT_RESET                = 11,
   kSNEEZE_ABI_METHOD_CONSOLE_TIME                       = 12,
   kSNEEZE_ABI_METHOD_CONSOLE_TIME_END                   = 13,
   kSNEEZE_ABI_METHOD_CONSOLE_TIME_LOG                   = 14,
};

enum eSNEEZE_ABI_METHOD_STORAGE
{
   kSNEEZE_ABI_METHOD_STORAGE_HAS                        =  1,
   kSNEEZE_ABI_METHOD_STORAGE_GET                        =  2,
   kSNEEZE_ABI_METHOD_STORAGE_SET                        =  3,
   kSNEEZE_ABI_METHOD_STORAGE_REMOVE                     =  4,
};

// NETWORK is the guest's request, socket, and Socket.IO API. It is deliberately
// laid out as three contiguous blocks with reserve space after each, so a method
// we did not think of can land beside its relatives instead of at the end of the
// enum:
//
//   1-29   REQUEST - an XHR-shaped HTTP request
//   30-59  SOCKET  - a browser-shaped WebSocket
//   60-89  IO      - a Socket.IO connection (host runs socket.io-client)
//
// A GET runs through the engine's cache exactly as an asset fetch does, so two
// guests asking for one URL share a single fetch. Every other verb is fetched
// fresh into transitory space and never reused. Either way the request is a
// first-class FILE, which is what makes it visible in the inspector.
//
// FETCH = 1 was declared here but never implemented and never sent, so the
// number is reserved rather than retired (see the numbering rule above) and
// REQUEST_OPEN takes it.
enum eSNEEZE_ABI_METHOD_NETWORK
{
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_OPEN               =  1,   // (eVerb, url, integrity) -> twRequestIx
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_HEADER_SET         =  2,   // set one request header (before SEND)
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_TIMEOUT_SET        =  3,   // per-request timeout in ms (before SEND)
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_SEND               =  4,   // issue it, with an optional body
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_ABORT              =  5,   // stop delivering to this caller
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_CLOSE              =  6,   // mirror of OPEN
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_STATE              =  7,   // -> eSNEEZE_ABI_REQUEST_STATE
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_STATUS             =  8,   // -> HTTP status code
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_STATUS_TEXT        =  9,   // -> reason phrase for the status
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_URL                = 10,   // -> final URL after redirects
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_HEADER_GET         = 11,   // -> one response header by name
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_HEADER_ALL         = 12,   // -> all response headers, CRLF-separated
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_BODY               = 13,   // -> response bytes
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_SIZE               = 14,   // -> response byte count
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_CONTENT_TYPE       = 15,   // -> content-type header
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_ERROR              = 16,   // -> transport error text, empty on success
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_IS_CACHED          = 17,   // -> bool, served from cache
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_COMPLETED          = 18,   // Notify: (twFabricIx, twRequestIx, bSuccess, 0)
   kSNEEZE_ABI_METHOD_NETWORK_REQUEST_PROGRESS           = 19,   // Notify: (twFabricIx, twRequestIx, nLoaded, nTotal) - not implemented yet (host new)
                                                                 // 20-29 reserved for REQUEST

   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_OPEN                = 30,   // (url, protocols) -> twSocketIx
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_SEND_TEXT           = 31,   // send a UTF-8 text frame
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_SEND_BINARY         = 32,   // send a binary frame
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_CLOSE               = 33,   // (wCode, reason) -> bool
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_STATE               = 34,   // -> eSNEEZE_ABI_SOCKET_STATE (readyState)
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_BUFFERED            = 35,   // -> bytes queued but unsent (bufferedAmount)
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_PROTOCOL            = 36,   // -> negotiated subprotocol
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_URL                 = 37,   // -> the socket's URL
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_RECV                = 38,   // -> pops the head of the receive queue
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_ERROR               = 39,   // -> last error text
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_OPENED              = 40,   // Notify: (twFabricIx, twSocketIx, 0, 0)
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_RECEIVED            = 41,   // Notify: (twFabricIx, twSocketIx, bBinary, nSize)
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_FAILED              = 42,   // Notify: (twFabricIx, twSocketIx, 0, 0)
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_CLOSED              = 43,   // Notify: (twFabricIx, twSocketIx, wCode, bClean)
   kSNEEZE_ABI_METHOD_NETWORK_SOCKET_FREE                = 44,   // retires the handle (CLOSE only closes the connection)
                                                                 // 45-59 reserved for SOCKET

   kSNEEZE_ABI_METHOD_NETWORK_IO_OPEN                    = 60,   // (url) -> twIoIx
   kSNEEZE_ABI_METHOD_NETWORK_IO_EMIT_TEXT               = 61,   // emit event + UTF-8 text (no ack)
   kSNEEZE_ABI_METHOD_NETWORK_IO_EMIT_BINARY             = 62,   // emit event + bytes (no ack)
   kSNEEZE_ABI_METHOD_NETWORK_IO_EMIT_TEXT_EX            = 63,   // emit event + UTF-8 text, ack with qwParam
   kSNEEZE_ABI_METHOD_NETWORK_IO_EMIT_BINARY_EX          = 64,   // emit event + bytes, ack with qwParam
   kSNEEZE_ABI_METHOD_NETWORK_IO_CLOSE                   = 65,   // Socket.IO disconnect; handle stays readable
   kSNEEZE_ABI_METHOD_NETWORK_IO_STATE                   = 66,   // -> eSNEEZE_ABI_IO_STATE
   kSNEEZE_ABI_METHOD_NETWORK_IO_BUFFERED                = 67,   // -> bytes queued but unsent
   kSNEEZE_ABI_METHOD_NETWORK_IO_URL                     = 68,   // -> the connection's URL
   kSNEEZE_ABI_METHOD_NETWORK_IO_RECV_EVENT              = 69,   // -> event name of event-queue head (does not pop)
   kSNEEZE_ABI_METHOD_NETWORK_IO_RECV                    = 70,   // -> pops event-queue payload
   kSNEEZE_ABI_METHOD_NETWORK_IO_RECV_ACK                = 71,   // -> pops ack-queue payload
   kSNEEZE_ABI_METHOD_NETWORK_IO_ERROR                   = 72,   // -> last error text
   kSNEEZE_ABI_METHOD_NETWORK_IO_OPENED                  = 73,   // Notify: (twFabricIx, twIoIx, 0, 0)
   kSNEEZE_ABI_METHOD_NETWORK_IO_RECEIVED                = 74,   // Notify: (twFabricIx, twIoIx, bBinary, nSize)
   kSNEEZE_ABI_METHOD_NETWORK_IO_FAILED                  = 75,   // Notify: (twFabricIx, twIoIx, 0, 0)
   kSNEEZE_ABI_METHOD_NETWORK_IO_CLOSED                  = 76,   // Notify: (twFabricIx, twIoIx, wCode, bClean)
   kSNEEZE_ABI_METHOD_NETWORK_IO_ACKED                   = 77,   // Notify: (twFabricIx, twIoIx, qwParam, nSize)
   kSNEEZE_ABI_METHOD_NETWORK_IO_FREE                    = 78,   // retires the handle (CLOSE only disconnects)
                                                                 // 79-89 reserved for IO
};

enum eSNEEZE_ABI_METHOD_VIEWPORT
{
   kSNEEZE_ABI_METHOD_VIEWPORT_POSITION_GET              =  1,        // not implemented yet (host new)
   kSNEEZE_ABI_METHOD_VIEWPORT_POSITION_SET              =  2,        // not implemented yet (host new)
   kSNEEZE_ABI_METHOD_VIEWPORT_ROTATION_GET              =  3,        // not implemented yet (host new)
   kSNEEZE_ABI_METHOD_VIEWPORT_ROTATION_SET              =  4,        // not implemented yet (host new)
};

enum eSNEEZE_ABI_METHOD_SCENE
{
   kSNEEZE_ABI_METHOD_SCENE_NODE_ROOT                    =  1,        // DEPRECATED - use FABRIC_NODE_ROOT instead
   kSNEEZE_ABI_METHOD_SCENE_NODE_MAP_DATA                =  2,        // DEPRECATED - use FABRIC_NODE_MAP_DATA instead
   kSNEEZE_ABI_METHOD_SCENE_NODE_OPEN                    =  3,        // DEPRECATED - use FABRIC_NODE_OPEN instead
   kSNEEZE_ABI_METHOD_SCENE_NODE_CLOSE                   =  4,        // DEPRECATED - use FABRIC_NODE_CLOSE instead
/*
   kSNEEZE_ABI_METHOD_SCENE_AMBIENT_GET                  =  1,        // not implemented yet (host new)
   kSNEEZE_ABI_METHOD_SCENE_AMBIENT_SET                  =  2,        // not implemented yet (host new)
   kSNEEZE_ABI_METHOD_SCENE_DIRECTIONAL_GET              =  3,        // not implemented yet (host new)
   kSNEEZE_ABI_METHOD_SCENE_DIRECTIONAL_SET              =  4,        // not implemented yet (host new)
   kSNEEZE_ABI_METHOD_SCENE_BACKGROUND_GET               =  5,        // not implemented yet (host new)
   kSNEEZE_ABI_METHOD_SCENE_BACKGROUND_SET               =  6,        // not implemented yet (host new)
*/
};

// FABRIC (type 7) builds and manages the fabric's node tree on its container.
// The first two methods hand the fabric to a browser-assigned map service
// (after which the guest no longer mutates its nodes directly); the remaining
// four are the guest-assigned node-tree calls. These six supersede the
// deprecated SCENE_NODE_* methods (type 6), which remain only so already-
// deployed modules keep working until they are migrated.
enum eSNEEZE_ABI_METHOD_FABRIC
{
   kSNEEZE_ABI_METHOD_FABRIC_NODE_MAP_SERVICE            =  1,        // connect a map service from a caller-filled SNEEZE_ABI_MAP_SERVICE
   kSNEEZE_ABI_METHOD_FABRIC_NODE_MAP_SERVICE_EX         =  2,        // connect a map service the host reads from the fabric's Services[name]
   kSNEEZE_ABI_METHOD_FABRIC_NODE_MAP_DATA               =  3,
   kSNEEZE_ABI_METHOD_FABRIC_NODE_ROOT                   =  4,
   kSNEEZE_ABI_METHOD_FABRIC_NODE_OPEN                   =  5,
   kSNEEZE_ABI_METHOD_FABRIC_NODE_CLOSE                  =  6,
};

enum eSNEEZE_ABI_METHOD_NODE
{
   kSNEEZE_ABI_METHOD_NODE_POSITION                      =  1,
   kSNEEZE_ABI_METHOD_NODE_ROTATION                      =  2,
   kSNEEZE_ABI_METHOD_NODE_SCALE                         =  3,
   kSNEEZE_ABI_METHOD_NODE_SCALE_AXES                    =  4,
   kSNEEZE_ABI_METHOD_NODE_BOUND                         =  5,
   kSNEEZE_ABI_METHOD_NODE_NAME                          =  6,
   kSNEEZE_ABI_METHOD_NODE_RESOURCE                      =  7,
   kSNEEZE_ABI_METHOD_NODE_PANEL                         =  8,
};

// CHRONO is the wall clock and the civil (calendar) logic for a MOMENT. The
// host owns all breakdown / formatting / parsing; the guest caches the filled
// SNEEZE_ABI_MOMENT and reads it locally. TIME/DATE return bare scalars; the
// rest fill a SNEEZE_ABI_MOMENT the guest supplied by (offset, length).
enum eSNEEZE_ABI_METHOD_CHRONO
{
   kSNEEZE_ABI_METHOD_CHRONO_TIME                        =  1,   // -> tm  (i64, 1/64 s since 1601, UTC)
   kSNEEZE_ABI_METHOD_CHRONO_DATE                        =  2,   // -> dt  (i64, Unix ms, UTC)
   kSNEEZE_ABI_METHOD_CHRONO_NOW                         =  3,   // fill MOMENT for "now"
   kSNEEZE_ABI_METHOD_CHRONO_MOMENT                      =  4,   // fill MOMENT from a tm or dt scalar
   kSNEEZE_ABI_METHOD_CHRONO_SET                         =  5,   // fill MOMENT from civil components (normalizes; every component setter routes here)
   kSNEEZE_ABI_METHOD_CHRONO_PARSE                       =  6,   // fill MOMENT from a string
   kSNEEZE_ABI_METHOD_CHRONO_FORMAT                      =  7,   // MOMENT + spec -> string
};

// PERFORMANCE is the monotonic high-resolution clock (JS performance.now).
// Values are 100 ns since a fixed origin; Origin fills the wall MOMENT at t0.
enum eSNEEZE_ABI_METHOD_PERFORMANCE
{
   kSNEEZE_ABI_METHOD_PERFORMANCE_NOW                    =  1,   // -> pf  (i64, 100 ns since origin, monotonic)
   kSNEEZE_ABI_METHOD_PERFORMANCE_ORIGIN                 =  2,   // fill MOMENT (wall anchor at t0)
};

// TIMER schedules one-shot and repeating callbacks. SET/CLEAR are guest -> host;
// FIRED is the host -> guest Notify event (the first event the ABI defines).
enum eSNEEZE_ABI_METHOD_TIMER
{
   kSNEEZE_ABI_METHOD_TIMER_SET                          =  1,   // arm (eUnit, nValue, qwParam, bRepeat) -> twTimerIx
   kSNEEZE_ABI_METHOD_TIMER_CLEAR                        =  2,   // disarm by twTimerIx
   kSNEEZE_ABI_METHOD_TIMER_FIRED                        =  3,   // Notify: (twFabricIx, twTimerIx, qwParam)
};

// SERVICES is the fabric's declared services, served read-only and on-demand,
// keyed by service name (the DATA model - one level by name rather than a
// dotted path). Get returns the named service's whole JSON object as text for
// the guest to parse itself.
enum eSNEEZE_ABI_METHOD_SERVICES
{
   kSNEEZE_ABI_METHOD_SERVICES_HAS                       =  1,
   kSNEEZE_ABI_METHOD_SERVICES_GET                       =  2,
};

// ---------------------------------------------------------------------------
// Shared enums - mirrors of the engine's own enums (kept in lockstep).
// ---------------------------------------------------------------------------

enum eSNEEZE_ABI_MAP_OBJECT_CLASS
{
   kSNEEZE_ABI_MAP_OBJECT_CLASS_ROOT                     = 70,
   kSNEEZE_ABI_MAP_OBJECT_CLASS_CELESTIAL                = 71,
   kSNEEZE_ABI_MAP_OBJECT_CLASS_TERRESTRIAL              = 72,
   kSNEEZE_ABI_MAP_OBJECT_CLASS_PHYSICAL                 = 73,
   kSNEEZE_ABI_MAP_OBJECT_CLASS_PANEL                    = 74,
   kSNEEZE_ABI_MAP_OBJECT_CLASS_LIGHT                    = 75,
};

enum eSNEEZE_ABI_SILO_SCOPE
{
   kSNEEZE_ABI_SILO_SCOPE_PERMANENT_ORG                  =  0,
   kSNEEZE_ABI_SILO_SCOPE_PERMANENT_CONTAINER            =  1,
   kSNEEZE_ABI_SILO_SCOPE_TEMPORARY_ORG                  =  2,
   kSNEEZE_ABI_SILO_SCOPE_TEMPORARY_CONTAINER            =  3,
};

enum eSNEEZE_ABI_TRUST
{
   kSNEEZE_ABI_TRUST_NONE                                =  0,
   kSNEEZE_ABI_TRUST_UNTRUSTED                           =  1,
   kSNEEZE_ABI_TRUST_UNVERIFIED                          =  2,
   kSNEEZE_ABI_TRUST_EXPIRED                             =  3,
   kSNEEZE_ABI_TRUST_VERIFIED                            =  4,
   kSNEEZE_ABI_TRUST_ROOT                                =  5,
};

// TIMER_SET's unit discriminant. TICK = TIMEX count (1/64 s); MS = milliseconds;
// HZ = frequency (period is 1/nValue seconds).
enum eSNEEZE_ABI_TIMER_UNIT
{
   kSNEEZE_ABI_TIMER_UNIT_TICK                           =  0,
   kSNEEZE_ABI_TIMER_UNIT_MS                             =  1,
   kSNEEZE_ABI_TIMER_UNIT_HZ                             =  2,
};

// REQUEST_OPEN's verb. GET is 0 because it is the default and the only verb
// that participates in the cache; the rest are always fetched fresh.
enum eSNEEZE_ABI_REQUEST_VERB
{
   kSNEEZE_ABI_REQUEST_VERB_GET                          =  0,
   kSNEEZE_ABI_REQUEST_VERB_POST                         =  1,
   kSNEEZE_ABI_REQUEST_VERB_PUT                          =  2,
   kSNEEZE_ABI_REQUEST_VERB_PATCH                        =  3,
   kSNEEZE_ABI_REQUEST_VERB_DELETE                       =  4,
   kSNEEZE_ABI_REQUEST_VERB_HEAD                         =  5,
};

// REQUEST_STATE, the analog of XHR's readyState. COMPLETE means the server
// answered, whatever it answered - a 404 is COMPLETE with a status of 404.
// FAILED means the transport never delivered a response at all.
enum eSNEEZE_ABI_REQUEST_STATE
{
   kSNEEZE_ABI_REQUEST_STATE_IDLE                        =  0,   // opened, not yet sent
   kSNEEZE_ABI_REQUEST_STATE_SENDING                     =  1,   // in flight
   kSNEEZE_ABI_REQUEST_STATE_COMPLETE                    =  2,   // answered
   kSNEEZE_ABI_REQUEST_STATE_FAILED                      =  3,   // transport failed
   kSNEEZE_ABI_REQUEST_STATE_ABORTED                     =  4,   // caller stopped listening
};

// SOCKET_STATE mirrors WebSocket.readyState exactly, values included.
enum eSNEEZE_ABI_SOCKET_STATE
{
   kSNEEZE_ABI_SOCKET_STATE_CONNECTING                   =  0,
   kSNEEZE_ABI_SOCKET_STATE_OPEN                         =  1,
   kSNEEZE_ABI_SOCKET_STATE_CLOSING                      =  2,
   kSNEEZE_ABI_SOCKET_STATE_CLOSED                       =  3,
};

// IO_STATE uses the same four values as SOCKET_STATE. A Socket.IO connection
// still connects, opens, closes, and ends; the names stay parallel so a guest
// that already branches on SOCKET_STATE can read IO_STATE the same way.
enum eSNEEZE_ABI_IO_STATE
{
   kSNEEZE_ABI_IO_STATE_CONNECTING                       =  0,
   kSNEEZE_ABI_IO_STATE_OPEN                             =  1,
   kSNEEZE_ABI_IO_STATE_CLOSING                          =  2,
   kSNEEZE_ABI_IO_STATE_CLOSED                           =  3,
};

// CHRONO zone selector: how SET interprets its civil input, and which cached
// view FORMAT renders. (Getters read both views straight from the MOMENT.)
enum eSNEEZE_ABI_CHRONO_ZONE
{
   kSNEEZE_ABI_CHRONO_ZONE_UTC                           =  0,
   kSNEEZE_ABI_CHRONO_ZONE_LOCAL                         =  1,
};

// ---------------------------------------------------------------------------
// OBJECTIX sentinels and composition (mirror of Map_Object.h / Scene.h).
// An OBJECTIX packs a 16-bit class in the high bits and a 48-bit object index
// in the low bits. OBJECTIX_IDENTITY as the index asks the engine to assign the
// next free per-container index ("P-?"). OBJECTIX_ERROR is the failure return.
// ---------------------------------------------------------------------------

#define SNEEZE_OBJECTIX_ERROR                             ((uint64_t) 0x0000FFFFFFFFFFFEull)
#define SNEEZE_OBJECTIX_IDENTITY                          ((uint64_t) 0x0000FFFFFFFFFFFFull)

#define SNEEZE_OBJECTIX_COMPOSE(wClass, twObjectIx)      (((uint64_t) (wClass) << 48)  |  ((uint64_t) (twObjectIx) & 0x0000FFFFFFFFFFFFull))
#define SNEEZE_OBJECTIX_CLASS(qwComposed)                ((uint16_t) ((qwComposed) >> 48))
#define SNEEZE_OBJECTIX_INDEX(qwComposed)                ((uint64_t) (qwComposed) & 0x0000FFFFFFFFFFFFull)

// ---------------------------------------------------------------------------
// SNEEZE_ABI_MAPOBJECT - the 528-byte binary wire struct for a map object. This
// is the one payload that stays raw binary (not field-serialized): builders fill
// it in guest memory, and node-create calls pass its (offset, length). The
// layout mirrors include/Map_Object.h field for field; the static assert guards
// drift. (The engine's internal name for this struct is still RMCOBJECT, being
// migrated off over time.)
// ---------------------------------------------------------------------------

#pragma pack(push, 1)
typedef struct tagSNEEZE_ABI_MAPOBJECT
{
   // OBJECT_HEAD (24 bytes)
   uint64_t                                                 qwComposed_Parent;
   uint64_t                                                 qwComposed_Self;
   uint64_t                                                 qwEvent;

   // MAP_OBJECT_NAME (96 bytes) - UTF-16 code units
   uint16_t                                                 wsName[48];

   // MAP_OBJECT_TYPE (8 bytes)
   uint8_t                                                  bType;
   uint8_t                                                  bSubtype;
   uint8_t                                                  bFiction;
   uint8_t                                                  abReserved_Type[5];

   // MAP_OBJECT_OWNER (8 bytes)
   uint64_t                                                 twOwner;

   // MAP_OBJECT_RESOURCE (200 bytes)
   uint64_t                                                 qwResource;
   char                                                     sName_Resource[64];
   char                                                     sReference[128];

   // MAP_OBJECT_TRANSFORM (80 bytes)
   double                                                   d3Position[3];
   double                                                   d4Rotation[4];
   double                                                   d3Scale[3];

   // MAP_OBJECT_ORBIT (32 bytes)
   int64_t                                                  tmPeriod;
   int64_t                                                  tmOrigin;
   double                                                   dA;
   double                                                   dB;

   // MAP_OBJECT_BOUND (48 bytes)
   uint8_t                                                  abReserved_Bound[24];
   double                                                   d3Max[3];

   // MAP_OBJECT_PROPERTIES (32 bytes)
   float                                                    fMass;
   float                                                    fGravity;
   float                                                    fColor;
   float                                                    fBrightness;
   float                                                    fReflectivity;
   uint8_t                                                  abReserved_Properties[12];
}
SNEEZE_ABI_MAPOBJECT, *PSNEEZE_ABI_MAPOBJECT;
#pragma pack(pop)

#define SNEEZE_ABI_MAPOBJECT_SIZE      528

// ---------------------------------------------------------------------------
// SNEEZE_ABI_MAP_SERVICE - the 592-byte binary wire struct for a map-service
// connection. Like MAPOBJECT it stays raw binary (not field-serialized) and
// flows guest -> host by (offset, length): the guest fills it (from its own
// knowledge, or from Services_Get(name) which it parses) and hands it to
// SCENE MAP_SERVICE; the MAP_SERVICE_EX variant instead names a service the
// host reads from the fabric's Services[name] and fills this struct itself.
// wClass/twObjectIx are kept adjacent (an OBJECTIX pair); abReserved pads the
// bAuth..wClass group to 8 bytes so twObjectIx lands 8-aligned.
// ---------------------------------------------------------------------------

#pragma pack(push, 1)
typedef struct tagSNEEZE_ABI_MAP_SERVICE
{
   char                                                     sNamespace[32];
   char                                                     sService[32];
   char                                                     sConnect[256];
   char                                                     sRootUrl[256];
   uint8_t                                                  bAuth;
   uint8_t                                                  abReserved[5];
   uint16_t                                                 wClass;
   uint64_t                                                 twObjectIx;
}
SNEEZE_ABI_MAP_SERVICE, *PSNEEZE_ABI_MAP_SERVICE;
#pragma pack(pop)

#define SNEEZE_ABI_MAP_SERVICE_SIZE    592

// ---------------------------------------------------------------------------
// SNEEZE_ABI_MOMENT - the guest-resident wall-clock value (CHRONO's MOMENT).
// Like MAPOBJECT it is a raw binary struct, but it flows host -> guest: the
// guest supplies a zeroed MOMENT by (offset, length) and the host fills it in
// one call - both scalar forms (tm, dt) plus the full UTC and local calendar
// breakdowns - so the guest reads Year/Month/Day/... locally without crossing
// back. A zeroed MOMENT (bMonth == 0) is the invalid sentinel. Windows
// SYSTEMTIME conventions: 1-based month, 0-based weekday (Sunday = 0). The
// sub-second is stored once, canonically, as dwFraction (100 ns units): 1/64 s
// and 1 ms both divide 100 ns evenly but not each other, so it is the only
// grain that round-trips both. tick and ms are derived views (tick =
// dwFraction/156250, ms = dwFraction/10000); tm and dt agree at whole seconds
// and differ only in that derived sub-second.
// ---------------------------------------------------------------------------

#pragma pack(push, 1)
typedef struct tagSNEEZE_ABI_CIVIL                       // one calendar breakdown
{
   int16_t                                                 wYear;      // full year (2026)
   uint8_t                                                 bMonth;     // 1-12 (7 = July); 0 = invalid
   uint8_t                                                 bDay;       // 1-31
   uint8_t                                                 bWeekday;   // 0-6 (0 = Sunday)
   uint8_t                                                 bHour;      // 0-23
   uint8_t                                                 bMinute;    // 0-59
   uint8_t                                                 bSecond;    // 0-59
   uint32_t                                                dwFraction; // sub-second, 100 ns units (0..9,999,999); tick = /156250, ms = /10000
}
SNEEZE_ABI_CIVIL, *PSNEEZE_ABI_CIVIL;

typedef struct tagSNEEZE_ABI_MOMENT
{
   int64_t                                                 tm;         // 1/64 s since 1601-01-01, UTC
   int64_t                                                 dt;         // Unix ms since 1970-01-01, UTC
   SNEEZE_ABI_CIVIL                                        Utc;        // UTC calendar breakdown
   SNEEZE_ABI_CIVIL                                        Local;      // local calendar breakdown
   int32_t                                                 nOffset;    // local offset from UTC, minutes
}
SNEEZE_ABI_MOMENT, *PSNEEZE_ABI_MOMENT;
#pragma pack(pop)

#define SNEEZE_ABI_CIVIL_SIZE          12
#define SNEEZE_ABI_MOMENT_SIZE         44

// ---------------------------------------------------------------------------
// Payload wire formats.
//
// Every payload is a sequence of little-endian scalar fields (no struct
// padding is assumed - fields are read/written in order). Conventions:
//   twFabricIx / qwComposed_Parent / qwComposed : u64 (the leading handle)
//   nXxxOffset, nXxxLen                  : i32 into the guest's linear memory
//                                          (a UTF-8 string, or MAPOBJECT bytes)
//   nOutOffset, nOutLen                  : i32 out-buffer for block returns;
//                                          Call returns the full size needed,
//                                          writing min(size, nOutLen) bytes.
//                                          nOutLen == 0 queries the size only.
//   scalars                              : i32 / i64 / f64 as noted
//
// The i64 Call return carries: a created composed identity, an action's 0/1 status,
// a boolean, or (for block getters) the full byte size needed.
//
//   DATA (twFabricIx, then...)  read-only; no scope
//     HAS    : (i32 nPathOffset, i32 nPathLen)                        -> bool
//     GET    : (i32 nPathOffset, i32 nPathLen, i32 nOutOffset, i32 nOutLen) -> size
//
//   CONSOLE (twFabricIx, then...)
//     LOG/DEBUG/INFO/WARN/ERROR/GROUP/GROUP_COLLAPSED/COUNT/COUNT_RESET/
//     TIME/TIME_END/TIME_LOG : (i32 nMsgOffset, i32 nMsgLen)
//     ASSERT                 : (i32 bCondition, i32 nMsgOffset, i32 nMsgLen)
//     GROUP_END              : (no further fields)
//
//   STORAGE (twFabricIx, i32 eScope, then...)
//     HAS    : (i32 nPathOffset, i32 nPathLen)                        -> bool
//     GET    : (i32 nPathOffset, i32 nPathLen, i32 nOutOffset, i32 nOutLen) -> size
//     SET    : (i32 nPathOffset, i32 nPathLen, i32 nValOffset, i32 nValLen) -> 0/1
//     REMOVE : (i32 nPathOffset, i32 nPathLen)                        -> 0/1
//
//   SCENE (DEPRECATED - identical wire to the matching FABRIC method; retained
//   only so already-deployed modules keep working)
//     NODE_ROOT          : deprecated
//     NODE_MAP_DATA      : deprecated
//     NODE_OPEN          : deprecated
//     NODE_CLOSE         : deprecated
//     AMBIENT_GET        : tbd
//     AMBIENT_SET        : tbd
//     DIRECTIONAL_GET    : tbd
//     DIRECTIONAL_SET    : tbd
//     BACKGROUND_GET     : tbd
//     BACKGROUND_SET     : tbd
//
//   FABRIC (the node-tree API; supersedes the deprecated SCENE_NODE_* methods)
//     NODE_MAP_SERVICE   : (u64 twFabricIx, i32 nSvcOffset, i32 nSvcLen)   -> 0/1  (nSvc points at a SNEEZE_ABI_MAP_SERVICE)
//     NODE_MAP_SERVICE_EX: (u64 twFabricIx, i32 nNameOffset, i32 nNameLen) -> 0/1  (host reads Services[name])
//     NODE_MAP_DATA      : (u64 twFabricIx, i32 nPathOffset, i32 nPathLen) -> qwComposed
//     NODE_ROOT          : (u64 twFabricIx, i32 nObjOffset, i32 nObjLen)   -> qwComposed
//     NODE_OPEN          : (i32 nObjOffset, i32 nObjLen)                   -> qwComposed  (parent read from the object's Head.Parent)
//     NODE_CLOSE         : (u64 qwComposed)                               -> 0/1
//
//   NODE (u64 qwComposed, then...)
//     POSITION   : (f64 dX, f64 dY, f64 dZ)
//     ROTATION   : (f64 dX, f64 dY, f64 dZ, f64 dW)   (quaternion)
//     SCALE      : (f64 dScale)
//     SCALE_AXES : (f64 dX, f64 dY, f64 dZ)
//     BOUND      : (f64 dX, f64 dY, f64 dZ)
//     NAME       : (i32 nNameOffset, i32 nNameLen)
//     RESOURCE   : (i32 nUrlOffset, i32 nUrlLen)
//     PANEL      : (i32 nRmlOffset, i32 nRmlLen)
//
//   CHRONO (u64 twFabricIx, then...)  host owns all civil logic; MOMENT is filled out
//     TIME   : ()                                                    -> i64 tm (1/64 s, 1601, UTC)
//     DATE   : ()                                                    -> i64 dt (Unix ms, UTC)
//     NOW    : (i32 nMomOffset, i32 nMomLen)                         -> 0/1  (fills MOMENT)
//     MOMENT : (i32 eSource, i64 qwValue, i32 nMomOffset, i32 nMomLen) -> 0/1  (eSource: 0 = tm, 1 = dt)
//     SET    : (i32 eZone, i32 wYear, i32 bMonth, i32 bDay, i32 bHour, i32 bMinute, i32 bSecond, i32 nFraction, i32 nMomOffset, i32 nMomLen) -> 0/1  (normalizes overflow; nFraction = sub-second in 100 ns units)
//     PARSE  : (i32 eZone, i32 nStrOffset, i32 nStrLen, i32 nMomOffset, i32 nMomLen) -> 0/1
//     FORMAT : (i32 eZone, i32 nSpecOffset, i32 nSpecLen, i32 nMomOffset, i32 nMomLen, i32 nOutOffset, i32 nOutLen) -> size
//
//   PERFORMANCE (u64 twFabricIx, then...)
//     NOW    : ()                                                    -> i64 pf (100 ns since origin, monotonic)
//     ORIGIN : (i32 nMomOffset, i32 nMomLen)                         -> 0/1  (wall MOMENT at t0)
//
//   TIMER (u64 twFabricIx, then...)
//     SET    : (i32 eUnit, i32 nValue, u64 qwParam, i32 bRepeat)     -> twTimerIx  (0 = failure)
//     CLEAR  : (u64 twTimerIx)                                       -> 0/1
//   TIMER Notify (host -> guest, packet handed to the Notify export):
//     FIRED  : (u64 twFabricIx, u64 twTimerIx, u64 qwParam)
//
//   SERVICES (twFabricIx, then...)  read-only; keyed by service name
//     HAS : (i32 nNameOffset, i32 nNameLen)                           -> bool
//     GET : (i32 nNameOffset, i32 nNameLen, i32 nOutOffset, i32 nOutLen) -> size
//
//   NETWORK / REQUEST. OPEN takes the fabric and returns a twRequestIx that
//   every later call carries instead. HEADER_SET and TIMEOUT_SET only take
//   effect before SEND. The getters are valid once COMPLETED has fired; before
//   that they report the request's current state and empty content.
//     REQUEST_OPEN         : (u64 twFabricIx, i32 eVerb, i32 nUrlOffset, i32 nUrlLen, i32 nIntegrityOffset, i32 nIntegrityLen) -> u64 twRequestIx  (0 = failure)
//     REQUEST_HEADER_SET   : (u64 twRequestIx, i32 nNameOffset, i32 nNameLen, i32 nValueOffset, i32 nValueLen) -> 0/1
//     REQUEST_TIMEOUT_SET  : (u64 twRequestIx, i32 nMilli)                 -> 0/1
//     REQUEST_SEND         : (u64 twRequestIx, i32 nBodyOffset, i32 nBodyLen) -> 0/1  (nBodyLen 0 = no body)
//     REQUEST_ABORT        : (u64 twRequestIx)                             -> 0/1
//     REQUEST_CLOSE        : (u64 twRequestIx)                             -> 0/1
//     REQUEST_STATE        : (u64 twRequestIx)                             -> eSNEEZE_ABI_REQUEST_STATE
//     REQUEST_STATUS       : (u64 twRequestIx)                             -> i64 HTTP status
//     REQUEST_SIZE         : (u64 twRequestIx)                             -> i64 byte count
//     REQUEST_IS_CACHED    : (u64 twRequestIx)                             -> bool
//     REQUEST_STATUS_TEXT  : (u64 twRequestIx, i32 nOutOffset, i32 nOutLen) -> size
//     REQUEST_URL          : (u64 twRequestIx, i32 nOutOffset, i32 nOutLen) -> size
//     REQUEST_HEADER_ALL   : (u64 twRequestIx, i32 nOutOffset, i32 nOutLen) -> size
//     REQUEST_BODY         : (u64 twRequestIx, i32 nOutOffset, i32 nOutLen) -> size
//     REQUEST_CONTENT_TYPE : (u64 twRequestIx, i32 nOutOffset, i32 nOutLen) -> size
//     REQUEST_ERROR        : (u64 twRequestIx, i32 nOutOffset, i32 nOutLen) -> size
//     REQUEST_HEADER_GET   : (u64 twRequestIx, i32 nNameOffset, i32 nNameLen, i32 nOutOffset, i32 nOutLen) -> size
//   NETWORK / REQUEST Notify (host -> guest). Every NETWORK notify field is a
//   u64, as TIMER_FIRED's are, so one packet builder serves them all.
//     REQUEST_COMPLETED    : (u64 twFabricIx, u64 twRequestIx, u64 bSuccess, u64 0)
//     REQUEST_PROGRESS     : (u64 twFabricIx, u64 twRequestIx, u64 nLoaded, u64 nTotal)
//
//   NETWORK / SOCKET. Same shape: OPEN takes the fabric and returns a handle.
//   RECV pops one message off the socket's receive queue - the guest calls it
//   after a RECEIVED notify, which carries the size so the guest can size its
//   buffer without a query call first. A guest that stops draining eventually
//   overflows the queue, and from there messages are dropped and SOCKET_ERROR
//   says so; nothing else about the socket changes.
//
//   The URL must be an absolute ws:// or wss:// URL. Unlike a request, it is not
//   resolved against the fabric - there is no relative form of a socket URL,
//   which is the same rule the browser's WebSocket constructor applies.
//
//   CLOSE closes the connection and leaves the handle readable, so a guest can
//   still ask what the close code was. FREE is what retires the handle.
//     SOCKET_OPEN          : (u64 twFabricIx, i32 nUrlOffset, i32 nUrlLen, i32 nProtoOffset, i32 nProtoLen) -> u64 twSocketIx  (0 = failure)
//     SOCKET_SEND_TEXT     : (u64 twSocketIx, i32 nOffset, i32 nLen)       -> 0/1
//     SOCKET_SEND_BINARY   : (u64 twSocketIx, i32 nOffset, i32 nLen)       -> 0/1
//     SOCKET_CLOSE         : (u64 twSocketIx, i32 wCode, i32 nReasonOffset, i32 nReasonLen) -> 0/1
//     SOCKET_STATE         : (u64 twSocketIx)                              -> eSNEEZE_ABI_SOCKET_STATE
//     SOCKET_BUFFERED      : (u64 twSocketIx)                              -> i64 bytes
//     SOCKET_PROTOCOL      : (u64 twSocketIx, i32 nOutOffset, i32 nOutLen) -> size
//     SOCKET_URL           : (u64 twSocketIx, i32 nOutOffset, i32 nOutLen) -> size
//     SOCKET_RECV          : (u64 twSocketIx, i32 nOutOffset, i32 nOutLen) -> size
//     SOCKET_ERROR         : (u64 twSocketIx, i32 nOutOffset, i32 nOutLen) -> size
//     SOCKET_FREE          : (u64 twSocketIx)                              -> 0/1
//   NETWORK / SOCKET Notify (host -> guest):
//     SOCKET_OPENED        : (u64 twFabricIx, u64 twSocketIx, u64 0, u64 0)
//     SOCKET_RECEIVED      : (u64 twFabricIx, u64 twSocketIx, u64 bBinary, u64 nSize)
//     SOCKET_FAILED        : (u64 twFabricIx, u64 twSocketIx, u64 0, u64 0)
//     SOCKET_CLOSED        : (u64 twFabricIx, u64 twSocketIx, u64 wCode, u64 bClean)
//
//   NETWORK / IO. A Socket.IO connection. The host runs official socket.io-client;
//   the guest sees a handle, two queues (events and acks), and five Notify hooks.
//   The URL must be an absolute http:// or https:// URL (the host may also accept
//   ws:// or wss://). It is not resolved against the fabric.
//
//   EMIT_TEXT / EMIT_BINARY are fire-and-forget. The _Ex forms request an ack and
//   echo qwParam on IO_ACKED (0 is a valid cookie). RECV_EVENT reads the event
//   name of the event-queue head without popping; RECV pops that payload.
//   RECV_ACK pops the ack-queue payload after IO_ACKED. CLOSE disconnects and
//   leaves the handle readable; FREE retires the handle.
//     IO_OPEN              : (u64 twFabricIx, i32 nUrlOffset, i32 nUrlLen) -> u64 twIoIx  (0 = failure)
//     IO_EMIT_TEXT         : (u64 twIoIx, i32 nEventOffset, i32 nEventLen, i32 nOffset, i32 nLen) -> 0/1
//     IO_EMIT_BINARY       : (u64 twIoIx, i32 nEventOffset, i32 nEventLen, i32 nOffset, i32 nLen) -> 0/1
//     IO_EMIT_TEXT_EX      : (u64 twIoIx, i32 nEventOffset, i32 nEventLen, i32 nOffset, i32 nLen, u64 qwParam) -> 0/1
//     IO_EMIT_BINARY_EX    : (u64 twIoIx, i32 nEventOffset, i32 nEventLen, i32 nOffset, i32 nLen, u64 qwParam) -> 0/1
//     IO_CLOSE             : (u64 twIoIx)                                  -> 0/1
//     IO_STATE             : (u64 twIoIx)                                  -> eSNEEZE_ABI_IO_STATE
//     IO_BUFFERED          : (u64 twIoIx)                                  -> i64 bytes
//     IO_URL               : (u64 twIoIx, i32 nOutOffset, i32 nOutLen)     -> size
//     IO_RECV_EVENT        : (u64 twIoIx, i32 nOutOffset, i32 nOutLen)     -> size
//     IO_RECV              : (u64 twIoIx, i32 nOutOffset, i32 nOutLen)     -> size
//     IO_RECV_ACK          : (u64 twIoIx, i32 nOutOffset, i32 nOutLen)     -> size
//     IO_ERROR             : (u64 twIoIx, i32 nOutOffset, i32 nOutLen)     -> size
//     IO_FREE              : (u64 twIoIx)                                  -> 0/1
//   NETWORK / IO Notify (host -> guest):
//     IO_OPENED            : (u64 twFabricIx, u64 twIoIx, u64 0, u64 0)
//     IO_RECEIVED          : (u64 twFabricIx, u64 twIoIx, u64 bBinary, u64 nSize)
//     IO_FAILED            : (u64 twFabricIx, u64 twIoIx, u64 0, u64 0)
//     IO_CLOSED            : (u64 twFabricIx, u64 twIoIx, u64 wCode, u64 bClean)
//     IO_ACKED             : (u64 twFabricIx, u64 twIoIx, u64 qwParam, u64 nSize)
//
// Flat C API name reference (the ergonomic C binding, layered on Call by
// sneeze.c): Console_Log/Debug/.../TimeLog; Storage_Has/Get/Set/Remove;
// Data_Has/Get; Network_Request_Open/Header_Set/Timeout_Set/Send/Abort/Close/State/
// Status/Status_Text/Url/Header/Headers/Body/Size/Content_Type/Error/Is_Cached and
// Network_Socket_Open/Send_Text/Send_Binary/Close/State/Buffered/Protocol/Url/Recv/
// Error/Free; Viewport_Position_Get/Set, Viewport_Rotation_Get/Set;
// Fabric_Node_Map_Service/Map_Service_Ex/Map_Data/Root/Open/Close (the node-tree
// API); Scene_Node_Root/Map_Data/Open/Close (DEPRECATED), Scene_Ambient_Get/Set,
// Scene_Directional_Get/Set, Scene_Background_Get/Set; Services_Has/Get; Node_Position/
// Rotation/Scale/Scale_Axes/Bound/Name/Resource/Panel. Every call takes the
// HOST* handle first; node methods also take HNODE; builders take HMAPOBJECT.
// ---------------------------------------------------------------------------

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SNEEZE_ABI_H
