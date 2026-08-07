# FABRIC

Node-tree construction on the fabric's container, reached through [`HOST::Fabric`](HOST.md#fabric). This is where a module builds what the user sees: it creates the fabric's root node and adds children, or hands the whole fabric to a browser-managed map service. `FABRIC` is a zero-cost view over the fabric handle.

The name mirrors the ABI's `FABRIC` subsystem (type 7). It is the node-tree construction view for a fabric: four node-building calls plus two map-service calls.

There are two ways to fill a fabric, and they are mutually exclusive per fabric:

- **Guest-assigned** - the module builds the node tree itself with `Node_Root` / `Node_Open` / `Node_Close` (or `Node_Map_Data` to build a subtree from the manifest's `Data` block). The building pattern is: fill a [`SNEEZE_ABI_MAPOBJECT`](MAPOBJECT.md) with a fluent builder, then hand it to `Node_Root` or `Node_Open`; each returns a live [`NODE`](NODE.md) you can mutate afterward.
- **Browser-assigned** - the module hands the fabric to a map service with `Node_Map_Service` / `Node_Map_Service_Ex`, and the browser drives the node tree from that service. **After a map-service call the module no longer mutates the fabric's nodes directly.**

## Methods

### Node_Map_Service

```rust
pub fn Node_Map_Service (&self, pService: &MAP_SERVICE) -> bool
```

- **Parameters:** `pService` - a `MAP_SERVICE`, the map-service connection struct, filled by the caller.
- **Returns:** `true` if the host accepted the connection request.
- **Description:** Connects a map service from a caller-filled `MAP_SERVICE`. Fill the struct from the module's own knowledge, or read a service definition with [`SERVICES::Get`](SERVICES.md#get), parse it, and copy the fields in (validating or overriding as you like). `MAP_SERVICE` is the raw 592-byte wire struct (`SNEEZE_ABI_MAP_SERVICE` in the ABI); the Rust SDK wraps it in a fluent builder (`Namespace`/`Service`/`Connect`/`RootUrl`/`Auth`/`Class`/`ObjectIx`). Once accepted, the browser manages the fabric's node tree, so do not also build nodes by hand.
- **Example:**

```rust
let mut pService = MAP_SERVICE::New ();
pService.Namespace ("com.rp1.map").Service ("rmap").Connect ("wss://map.rp1.com").Auth (true);
pHost.Fabric ().Node_Map_Service (&pService);
```

- **See also:** [`Node_Map_Service_Ex`](#node_map_service_ex), [`SERVICES`](SERVICES.md).

### Node_Map_Service_Ex

```rust
pub fn Node_Map_Service_Ex (&self, sName: &str) -> bool
```

- **Parameters:** `sName` - the name of a service declared in the fabric's `Services` block (`"Map"`).
- **Returns:** `true` if the host accepted the connection request.
- **Description:** Connects a map service the **host** reads from the fabric's `Services[name]` and fills the struct itself - the module supplies only the name, letting the browser read the service definition on its own. Use this when the module does not need to inspect or modify the connection details. As with `Node_Map_Service`, the browser manages the node tree afterward.
- **Example:**

```rust
pHost.Fabric ().Node_Map_Service_Ex ("Map");
```

- **See also:** [`Node_Map_Service`](#node_map_service), [`SERVICES`](SERVICES.md).

### Node_Map_Data

```rust
pub fn Node_Map_Data (&self, sPath: &str) -> NODE
```

- **Parameters:** `sPath` - a path into the fabric's `Data` block (`""` = the `Data` object itself).
- **Returns:** the created root [`NODE`](NODE.md) of the built subtree.
- **Description:** Asks the engine to read a node description from the fabric's `Data` block at `sPath` and build the entire subtree from it, without you constructing each map object by hand. This is the "map-managed" path a data-driven fabric uses. See [`DATA`](DATA.md) for reading that same block as raw values.
- **Example:**

```rust
let pRoot = pHost.Fabric ().Node_Map_Data ("Scene");
```

- **See also:** [`DATA`](DATA.md), [`Node_Root`](#node_root).

### Node_Root

```rust
pub fn Node_Root (&self, pObject: &SNEEZE_ABI_MAPOBJECT) -> NODE
```

- **Parameters:** `pObject` - the map object describing the root node (built with a [`SNEEZE_ABI_MAPOBJECT`](MAPOBJECT.md) builder).
- **Returns:** the created root [`NODE`](NODE.md). Check `NODE::IsValid ()`.
- **Description:** Creates the fabric's single root node from a map object. A fabric has one root; everything else is a descendant of it. The create call copies the map object's bytes, so you may drop or reuse the builder afterward.
- **Example:**

```rust
let mut pRoot = SNEEZE_ABI_MAPOBJECT::Physical ();
pRoot.Name ("Stool").Reference ("assets/Stool.glb");
let pNode = pHost.Fabric ().Node_Root (&pRoot);
```

- **See also:** [`Node_Open`](#node_open), [`SNEEZE_ABI_MAPOBJECT`](MAPOBJECT.md), [`NODE`](NODE.md).

### Node_Open

```rust
pub fn Node_Open (&self, pObject: &SNEEZE_ABI_MAPOBJECT) -> NODE
```

- **Parameters:**
  - `pObject` - the map object describing the child node. Its parent is taken from the object's own parent index, set with [`SNEEZE_ABI_MAPOBJECT::Parent`](MAPOBJECT.md#parent).
- **Returns:** the created child [`NODE`](NODE.md). Check `NODE::IsValid ()`.
- **Description:** Creates a child node from a map object. The parent is whatever parent index you placed in the object's Head - no parent `NODE` handle is required, so you may name any parent index directly (useful when authoring a flat tree by explicit index). When you have the parent as a live `NODE`, set the child's parent from it with `Parent (pParent.Class (), pParent.ObjectIx ())`. The create call copies the map object's bytes.
- **Example:**

```rust
let pFabric = pHost.Fabric ();

let mut pRoot = SNEEZE_ABI_MAPOBJECT::Physical ();
pRoot.Name ("Stool").Reference ("assets/Stool.glb");
let pStool = pFabric.Node_Root (&pRoot);

let mut pChild = SNEEZE_ABI_MAPOBJECT::Physical ();
pChild.Parent (pStool.Class (), pStool.ObjectIx ()).Name ("Bucket").Reference ("assets/Bucket.glb").Position (0.0, 0.0, 0.428);
let pBucket = pFabric.Node_Open (&pChild);
```

- **See also:** [`Node_Root`](#node_root), [`Node_Close`](#node_close).

### Node_Close

```rust
pub fn Node_Close (&self, pNode: NODE) -> bool
```

- **Parameters:** `pNode` - the node to remove.
- **Returns:** `true` on success.
- **Description:** Removes and deletes a node (and its descendants) from the scene. Use it to tear down something you created earlier.
- **Example:**

```rust
pHost.Fabric ().Node_Close (pBucket);
```

- **See also:** [`Node_Open`](#node_open).

## See also

- [HOST](HOST.md) - the root handle this view is reached through.
- [SNEEZE_ABI_MAPOBJECT](MAPOBJECT.md) - the builder you fill in and hand to `Node_Root` / `Node_Open`.
