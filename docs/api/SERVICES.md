# SERVICES

The fabric's declared services, reached through [`HOST::Services`](HOST.md#services). A service is a discrete unit of functionality the fabric author declared in the manifest's `Services` block. `SERVICES` is a zero-cost view over the fabric handle.

`SERVICES` is a **name-keyed sibling of [`DATA`](DATA.md)**: the same read-only, JSON-text model, but addressed by service *name* (`"Map"`) rather than a dotted path. A service object may carry **any fields the fabric author chose**, so `Get` returns the named service's whole JSON object as text for the module to parse itself.

Like `Data`, the `Services` block is deliberately *not* included in the [`Open`](INSTANCE.md#open) snapshot. It is served on demand and stays available for the whole life of the fabric, not just during `Open`.

## Methods

### Has

```rust
pub fn Has (&self, sName: &str) -> bool
```

- **Parameters:** `sName` - the service name (`"Map"`).
- **Returns:** `true` if a service is declared under that name.
- **Description:** Tests for the presence of a named service without reading it.
- **Example:**

```rust
if pHost.Services ().Has ("Map")
{
   // the fabric declares a Map service
}
```

- **See also:** [`Get`](#get), [`DATA::Has`](DATA.md#has).

### Get

```rust
pub fn Get (&self, sName: &str) -> Option<String>
```

- **Parameters:** `sName` - the service name (`"Map"`).
- **Returns:** `Some(json)` with the whole service object as JSON text, or `None` for an absent service.
- **Description:** Reads the named service's entire object as a JSON string. Sizes the result buffer in one probe and, if needed, one exact re-read. Parse the returned string with your JSON library of choice, then act on the fields (for a map service, fill a [`MAP_SERVICE`](FABRIC.md#node_map_service) and hand it to [`FABRIC::Node_Map_Service`](FABRIC.md#node_map_service)).
- **Example:**

```rust
if let Some (sJson) = pHost.Services ().Get ("Map")
{
   pHost.Console ().Log (&sJson);
}
```

- **See also:** [`Has`](#has), [`DATA::Get`](DATA.md#get), [`FABRIC::Node_Map_Service_Ex`](FABRIC.md#node_map_service_ex) (let the host read the service itself).

## See also

- [DATA](DATA.md) - the dotted-path sibling with the same read-only JSON shape.
- [FABRIC::Node_Map_Service](FABRIC.md#node_map_service) / [FABRIC::Node_Map_Service_Ex](FABRIC.md#node_map_service_ex) - connect a map service from a service definition.
- [MODULE](MODULE.md) - the declared WASM modules a fabric references.
