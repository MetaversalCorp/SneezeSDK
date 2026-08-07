# HOST

The root handle for one open fabric. You receive a `HOST` at [`Open`](INSTANCE.md#open) - a `&HOST` in Rust, a `HOST*` in C/C++ - and every subsystem you can use hangs off it. The SDK allocates one `HOST` per fabric at `Open`, keeps it stable for that fabric's whole life, and frees it at `Close`, so the handle you are given stays valid across callbacks (a timer fire hands you the same `HOST`). It wraps just the fabric index, so it is cheap to pass around.

The subsystem accessors (`Console`, `Storage`, `Data`, `Services`, `Scene`, `Fabric`) return **zero-cost views**: each wraps the same fabric index, allocates nothing, and knows which fabric it routes to. There is one of each subsystem per fabric.

> The root handle used to be called `FABRIC`. It was renamed to `HOST` so the name `FABRIC` could be reused for the node-tree subsystem view (below), which mirrors the ABI's `FABRIC` type. Node construction lives on [`HOST::Fabric`](#fabric).

## Methods

### Index

```rust
pub fn Index (&self) -> u64
```

- **Parameters:** none.
- **Returns:** the fabric's index (its handle value).
- **Description:** The stable identity of this fabric for the life of the open. Because one WASM instance can hold several fabrics open at once, use this to key any per-fabric state you retain across `Open`/`Close`.
- **Example:**

```rust
let nKey = pHost.Index ();
```

- **See also:** [`Open`](INSTANCE.md#open), [`Close`](INSTANCE.md#close).

### Console

```rust
pub fn Console (&self) -> CONSOLE
```

- **Parameters:** none.
- **Returns:** the [`CONSOLE`](CONSOLE.md) view for this fabric.
- **Description:** Developer-console logging, forwarded to the container's log stream.
- **Example:**

```rust
pHost.Console ().Log ("hello");
```

- **See also:** [`CONSOLE`](CONSOLE.md).

### Storage

```rust
pub fn Storage (&self) -> STORAGE
```

- **Parameters:** none.
- **Returns:** the [`STORAGE`](STORAGE.md) view for this fabric.
- **Description:** Persistent JSON document storage, scoped per persona and organization. Use it to save and restore module state across sessions.
- **Example:**

```rust
let bHas = pHost.Storage ().Has (eSNEEZE_ABI_SILO_SCOPE::kSNEEZE_ABI_SILO_SCOPE_PERMANENT_CONTAINER, "settings");
```

- **See also:** [`STORAGE`](STORAGE.md).

### Data

```rust
pub fn Data (&self) -> DATA
```

- **Parameters:** none.
- **Returns:** the [`DATA`](DATA.md) view for this fabric.
- **Description:** The fabric's read-only configuration `Data` tree, served on demand by path. The immutable analog of storage.
- **Example:**

```rust
let sTitle = pHost.Data ().Get ("Scene.sTitle");
```

- **See also:** [`DATA`](DATA.md).

### Services

```rust
pub fn Services (&self) -> SERVICES
```

- **Parameters:** none.
- **Returns:** the [`SERVICES`](SERVICES.md) view for this fabric.
- **Description:** The fabric's declared services, served read-only on demand by service name. A name-keyed sibling of `Data`; each service object may carry any fields the author chose.
- **Example:**

```rust
if let Some (sJson) = pHost.Services ().Get ("Map")
{
   pHost.Console ().Log (&sJson);
}
```

- **See also:** [`SERVICES`](SERVICES.md).

### Fabric

```rust
pub fn Fabric (&self) -> FABRIC
```

- **Parameters:** none.
- **Returns:** the [`FABRIC`](FABRIC.md) node-tree view for this fabric.
- **Description:** Node-tree construction on the fabric's container - create the root, add children, remove nodes, or hand the fabric to a browser-managed map service. This is the subsystem a module uses to build what the user sees.
- **Example:**

```rust
let mut pRoot = SNEEZE_ABI_MAPOBJECT::Root ();
let pNode = pHost.Fabric ().Node_Root (&pRoot);
```

- **See also:** [`FABRIC`](FABRIC.md), [`SNEEZE_ABI_MAPOBJECT`](MAPOBJECT.md).

### Scene

```rust
pub fn Scene (&self) -> SCENE
```

- **Parameters:** none.
- **Returns:** the `SCENE` view for this fabric.
- **Description:** Scene-global state - ambient light, directional light, and background. **Reserved:** its Ambient/Directional/Background get/set methods are not implemented yet, so the view exposes none for now. It exists so `pHost.Scene ()` is ready when they land.
- **See also:** [`Fabric`](#fabric).

### Chrono

```rust
pub fn Chrono (&self) -> CHRONO
```

- **Parameters:** none.
- **Returns:** the [`CHRONO`](CHRONO.md) view.
- **Description:** The wall clock. Read the current instant as a scalar (`Time`, `Date`) or as a [`MOMENT`](MOMENT.md) calendar value (`Now`). The clock is global, so this reports the same instant for every fabric.
- **Example:**

```rust
let m = pHost.Chrono ().Now ();
```

- **See also:** [`CHRONO`](CHRONO.md), [`MOMENT`](MOMENT.md).

### Performance

```rust
pub fn Performance (&self) -> PERFORMANCE
```

- **Parameters:** none.
- **Returns:** the [`PERFORMANCE`](PERFORMANCE.md) view.
- **Description:** The monotonic high-resolution clock. Use it to measure elapsed durations rather than to read the calendar.
- **Example:**

```rust
let nStart = pHost.Performance ().Now ();
```

- **See also:** [`PERFORMANCE`](PERFORMANCE.md).

### Timer

```rust
pub fn Timer (&self) -> TIMER
```

- **Parameters:** none.
- **Returns:** the [`TIMER`](TIMER.md) view.
- **Description:** Arm one-shot (`Set`) or repeating (`Interval`) callbacks; the fire arrives at [`INSTANCE::Timer`](INSTANCE.md#timer). Timers disarm automatically when the fabric closes.
- **Example:**

```rust
pHost.Timer ().Interval (1, eSNEEZE_ABI_TIMER_UNIT::kSNEEZE_ABI_TIMER_UNIT_HZ, 0);
```

- **See also:** [`TIMER`](TIMER.md), [`INSTANCE::Timer`](INSTANCE.md#timer).

## Snapshot views

Beyond the live subsystems above, `HOST` also exposes the fabric's *immutable* configuration - the [Open snapshot](SNAPSHOT.md) the engine pushed at `Open`. The SDK parses that snapshot once, privately; a module never sees the raw JSON. Each accessor below returns a read-only, typed view of one section. `Resource`/`Signature`/`Agent`/`Container` and the `Modules` list borrow directly from the parsed snapshot (no copy, no boundary crossing); `Location` is computed on demand from the resource reference. (Declared *services* are **not** in the snapshot - they are served on demand through the [`Services`](#services) subsystem above.)

| Accessor | Returns | Page |
|----------|---------|------|
| `Location ()` | [`LOCATION`](LOCATION.md) | the fabric URL, split into parts (a `window.location` analog) |
| `Resource ()` | `&RESOURCE` | [RESOURCE](RESOURCE.md) - the launching resource's identity |
| `Container ()` | `&CONTAINER` | [CONTAINER](CONTAINER.md) - container identity and trust |
| `Signature ()` | `&SIGNATURE` | [SIGNATURE](SIGNATURE.md) - MSF verification detail |
| `Agent ()` | `&AGENT` | [AGENT](AGENT.md) - host/engine identity (a `navigator` analog) |
| `Modules ()` | `&[MODULE]` | [MODULE](MODULE.md) - declared WASM modules |

```rust
pHost.Console ().Log (pHost.Container ().Name ());
pHost.Console ().Log (pHost.Location ().Host ());

for pModule in pHost.Modules ()
{
   pHost.Console ().Log (pModule.Url ());
}
```

## See also

- [API overview](overview.md) - how `HOST` roots the whole object model.
- [FABRIC](FABRIC.md) - the node-tree subsystem reached through [`Fabric`](#fabric).
- [the Open snapshot](SNAPSHOT.md) - the immutable configuration behind the snapshot views.
