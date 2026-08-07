# Sneeze WASM Module SDK

This is the reference for writing a WASM module that runs inside the Sneeze engine. A module is a small, sandboxed program (compiled to WebAssembly) that a spatial fabric loads to build and drive its scene: it creates nodes, reacts to lifecycle events, reads its configuration, stores state, and writes to the developer console.

A module talks to the engine across one narrow, frozen interface (the ABI). You almost never touch that interface directly. Instead you use a per-language SDK that wraps it in ordinary typed objects. SDKs exist for C, C++, and Rust; the ABI is language-neutral, so more languages can follow. The examples in this reference are written in Rust, but the method semantics are identical across the bindings (in C the calls are flat functions that take the `HOST*` explicitly, e.g. `Console_Log (pHost, ...)`; in C++ and Rust they hang off the handle, e.g. `pHost->Console ().Log (...)`).

## Where to start

1. [Incorporating the ABI](incorporating-the-abi.md) - how to set up a project in each language, what the engine imports and exports, and the wire format underneath the SDK.
2. [API overview](api/overview.md) - the object model: the `HOST` root, the `Open` lifecycle, how the pieces fit together.
3. The per-class pages below - one page per class, each documenting every method with its declaration, parameters, return value, a description, an example, and related methods.

Writing in C? See [The C SDK](c-sdk.md) for the Emscripten setup and the full flat C function reference; the per-class pages below still describe the semantics.

## API reference

Lifecycle and core:

- [INSTANCE](api/INSTANCE.md) - the trait your module implements (`Init`, `Open`, `Close`, `Shutdown`) and the `instance!` macro that wires it up.
- [HOST](api/HOST.md) - the root handle; every subsystem and snapshot view hangs off it.
- [The Open snapshot](api/SNAPSHOT.md) - the immutable configuration pushed to your module at `Open`, and how you read it.

Subsystems (reached through `HOST`):

- [CONSOLE](api/CONSOLE.md) - developer-console logging.
- [STORAGE](api/STORAGE.md) - persistent, per-scope JSON document storage.
- [DATA](api/DATA.md) - the fabric's read-only configuration data tree.
- [SERVICES](api/SERVICES.md) - the fabric's declared services, served on demand by name.
- [FABRIC](api/FABRIC.md) - node-tree construction (and map-service attach).
- [NODE](api/NODE.md) - per-node property mutation.

Time and scheduling (reached through `HOST`):

- [CHRONO](api/CHRONO.md) - the wall clock (a `Date`-namespace analog).
- [MOMENT](api/MOMENT.md) - a wall-clock instant value (a `Date`-instance analog).
- [PERFORMANCE](api/PERFORMANCE.md) - the monotonic clock, for elapsed timing.
- [TIMER](api/TIMER.md) - one-shot and repeating scheduled callbacks.

Scene building:

- [SNEEZE_ABI_MAPOBJECT](api/MAPOBJECT.md) - the fluent builder you fill in and hand to `FABRIC` to create a node.

Snapshot views (read from the `Open` snapshot through `HOST`):

- [LOCATION](api/LOCATION.md) - the fabric's address (a `window.location` analog).
- [RESOURCE](api/RESOURCE.md) - the launching resource's identity.
- [CONTAINER](api/CONTAINER.md) - the container identity and trust standing.
- [SIGNATURE](api/SIGNATURE.md) - the MSF signature-verification result.
- [AGENT](api/AGENT.md) - host/engine identity (a `navigator` analog).
- [MODULE](api/MODULE.md) - a declared WASM module from the fabric manifest.

## Conventions used in this reference

- Type and class names are ALL CAPS (`FABRIC`, `NODE`, `SNEEZE_ABI_MAPOBJECT`).
- Function and method names are TitleCase (`Node_Root ()`, `Console ()`).
- Parameters carry a type prefix (Hungarian): `s` string, `n` number, `d` double, `b` bool, `tw`/`qw` 64-bit, `dw` 32-bit, `p` pointer/object.
- Names are kept identical across languages and across the engine wherever the same concept exists.
- Code examples on the per-class pages are Rust. The prose is language-neutral; a C author reads the same method semantics and applies them through the [C binding](c-sdk.md) (which lists every C signature), and a Go or other author applies them through that language's binding.
