# API Overview

This page ties the SDK classes together. Read it once before the per-class pages; it explains the object model, the module lifecycle, and how a module is structured.

## The object model is rooted at HOST

A running module is a **WASM instance**. One instance can serve **many fabrics** - the engine calls your `Open` once per fabric that loads your module. Everything you can do for a given fabric hangs off the [`HOST`](HOST.md) handle you are given at `Open` (a `&HOST` in Rust, a `HOST*` in C/C++). The SDK creates one `HOST` per fabric at `Open`, keeps it stable for that fabric's life, and frees it at `Close`:

```
HOST (the root handle for one open fabric)
- Console ()     -> CONSOLE      developer-console logging
- Storage ()     -> STORAGE      persistent JSON, per scope
- Data ()        -> DATA         the fabric's read-only config "Data" tree
- Services ()    -> SERVICES     the fabric's declared services, by name
- Fabric ()      -> FABRIC       build the node tree (or attach a map service)
- Chrono ()      -> CHRONO       the wall clock (and the MOMENT calendar value)
- Performance () -> PERFORMANCE  the monotonic clock, for elapsed timing
- Timer ()       -> TIMER        one-shot / repeating scheduled callbacks
```

The subsystem objects (`CONSOLE`, `STORAGE`, `DATA`, `SERVICES`, `FABRIC`, `CHRONO`, `PERFORMANCE`, `TIMER`) are **zero-cost views**: each is just the fabric index in a typed wrapper. Calling `Console ()` allocates nothing; it hands back a handle that knows which fabric it belongs to. This is why there is one `Console_Log` and not a `Host_Console` round-trip - the fabric handle *is* the routing key.

There is exactly one of each subsystem per fabric (they are singletons), which is why they need no identifier beyond the fabric. Nodes are different: there are many per fabric, so a [`NODE`](NODE.md) carries its own object index and is mutated by that index.

## The Open snapshot

At `Open` the engine also pushes an immutable [Open snapshot](SNAPSHOT.md): a JSON document it synthesized for this fabric, copied into your memory through the ABI's `Alloc`/`Open`/`Free` handshake. The SDK parses it **once, privately** - before your `Open` runs - so a module never touches the raw JSON or its transient buffer. You read the parsed sections through the same [`HOST`](HOST.md) handle:

```
HOST (also exposes the immutable snapshot)
- Location ()   -> LOCATION    the fabric URL, split into parts
- Resource ()   -> RESOURCE    the launching resource's identity
- Container ()  -> CONTAINER   container identity and trust
- Signature ()  -> SIGNATURE   MSF verification detail
- Agent ()      -> AGENT       host/engine identity (a navigator analog)
- Modules ()    -> [MODULE]    declared WASM modules
```

`LOCATION` is derived guest-side by splitting the launching resource's reference; the rest are read straight from the parsed snapshot.

The fabric's open-ended configuration `Data` block is **not** in the snapshot. It is served on demand, read-only, through [`DATA`](DATA.md) - the same path-addressed model as storage, so a large data block never inflates the `Open` blob. The fabric's declared **services** are likewise served on demand (by name) through [`SERVICES`](SERVICES.md), not carried in the snapshot.

## The module lifecycle

You implement the [`INSTANCE`](INSTANCE.md) trait and register it with the `instance!` macro. The engine drives it:

```
Init ()                        once, when the module is first loaded
Open (pHost)                   once per fabric that loads the module
   ... the fabric is live: build nodes, read data, store state ...
Close (pHost)                  that fabric unloaded
Shutdown ()                    the module is unloading
```

`Init` and `Shutdown` bracket the module's whole life. Each `Open`/`Close` pair brackets one fabric. Because one instance can hold several fabrics open at once, never assume `Open` and `Close` are one-to-one in time - key any per-fabric state by `HOST::Index ()` (the `HOST*` you are given is itself stable per fabric, so it also serves as a key).

Beyond these four, the engine can also call your instance back *asynchronously* through [`INSTANCE::Timer`](INSTANCE.md#timer) when a [`TIMER`](TIMER.md) you armed fires. These callbacks arrive between `Open` and `Close` for the fabric that armed the timer.

## Time, timers, and the clock

Three fabric views cover time, split by role rather than lumped into one `Date`:

- [`CHRONO`](CHRONO.md) is the **wall clock** - "what time is it?" It returns scalars (`Time`, `Date`) and a [`MOMENT`](MOMENT.md) calendar value (`Now`).
- [`PERFORMANCE`](PERFORMANCE.md) is the **monotonic clock** - "how much time elapsed?" It never runs backward, so it is what you time work with.
- [`TIMER`](TIMER.md) is the **scheduler** - arm a one-shot or repeating callback instead of polling the clock.

A [`MOMENT`](MOMENT.md) is a value you hold by copy; the host fills both scalar forms and both (UTC and local) calendar breakdowns in one call, so reading it back never crosses the boundary again.

## Building a scene

Scene construction is a two-step pattern:

1. Build a [`SNEEZE_ABI_MAPOBJECT`](MAPOBJECT.md) in guest memory with a fluent builder - pick a class factory (`Physical ()`, `Celestial ()`, `Light ()`, ...), then chain setters (`Name`, `Reference`, `Position`, ...).
2. Hand it to [`FABRIC`](FABRIC.md): `Node_Root (&obj)` for the fabric root, or `Node_Open (&obj)` for a child (set the child's parent index on the object first). Each returns a [`NODE`](NODE.md).

After creation you mutate a live node through its `NODE` handle (`Position`, `Scale`, `Name`, `Resource`, `Panel`, ...). The map object is the *creation* template; the node is the *live* object.

`FABRIC::Node_Map_Data (sPath)` is a shortcut: it asks the engine to build an entire node subtree directly from a path in the fabric's `Data` block, without you constructing each map object. Alternatively, [`FABRIC::Node_Map_Service`](FABRIC.md#node_map_service) hands the whole fabric to a browser-managed map service instead of building nodes by hand.

## What is not here yet

The ABI reserves numbers for subsystems and methods that are declared but not yet implemented in the engine: `NETWORK` (fetch), `VIEWPORT` (camera get/set), the `SCENE` global-lighting and background methods, and `NODE::Rotation`. These appear in `sneeze_abi.h` marked "not implemented yet" and have no SDK wrapper - do not rely on them until they land.

Event delivery through `Notify` is live: it carries [`TIMER`](TIMER.md) fires today, dispatched to [`INSTANCE::Timer`](INSTANCE.md#timer). Other event kinds (node events, for example) are still reserved; a `Notify` packet a module has no hook for is simply ignored, so old modules stay forward-compatible as new event kinds land.

## See also

- [Incorporating the ABI](../incorporating-the-abi.md) - the wire format beneath these objects and how to set up a project.
- [HOST](HOST.md) - the entry point every example starts from.
- [FABRIC](FABRIC.md) - the node-tree subsystem reached through `HOST::Fabric`.
- [INSTANCE](INSTANCE.md) - the lifecycle you implement.
