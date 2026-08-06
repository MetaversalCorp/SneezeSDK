# The C SDK

The C binding for writing Sneeze WASM modules, built with Emscripten. It lives in
the `SneezeSDK_C` repository and layers directly on the canonical ABI header
`sdk/include/sneeze_abi.h`, mirroring the Rust SDK function for function.

The object model, the `Open` lifecycle, and every method's semantics are exactly
those documented in the [API overview](api/overview.md) and the per-class pages -
this page does not repeat them. It covers what is specific to C: how the object
model maps onto a flat set of functions, how you implement the lifecycle, how
memory and strings are handled, and how to build. Read the [API overview](api/overview.md)
first for the concepts; use the per-class pages (linked throughout) for the
meaning of each call.

## The shape of the C binding

C has no methods, so the binding is **flat**: every subsystem method becomes a
free function whose first argument is the handle that Rust would have called the
method on. The canonical names are fixed by `sneeze_abi.h` itself.

- Singleton subsystems take an `HFABRIC` (the fabric handle):
  `pFabric.Console ().Log (s)` becomes `Console_Log (twFabricIx, s)`.
- A node takes its own `HNODE` (a composed object index):
  `pNode.Position (x, y, z)` becomes `Node_Position (qwComposed, x, y, z)`.
- The map-object builder is an `HMAPOBJECT` (an opaque handle to a guest-local
  builder): `SNEEZE_ABI_MAPOBJECT::Physical ()` becomes `MapObject_Physical ()`.

| Rust                                   | C                                             |
|----------------------------------------|-----------------------------------------------|
| `pFabric.Console ().Log ("hi")`        | `Console_Log (twFabricIx, "hi")`              |
| `pFabric.Storage ().Has (eScope, p)`   | `Storage_Has (twFabricIx, eScope, p)`         |
| `pFabric.Data ().Get (p)`              | `Data_Get (twFabricIx, p)`                    |
| `pFabric.Scene ().Node_Root (&obj)`    | `Scene_Node_Root (twFabricIx, pObject)`       |
| `pNode.Scale (2.0)`                    | `Node_Scale (qwComposed, 2.0)`                |
| `pFabric.Container ().Name ()`         | `Container_Name (twFabricIx)`                 |
| `SNEEZE_ABI_MAPOBJECT::Physical ()`    | `MapObject_Physical ()`                       |

The handle types come straight from `sneeze_abi.h`: `HFABRIC` and `HNODE` are
64-bit value handles, `HMAPOBJECT` is a 32-bit builder handle. Include one header:

```c
#include "sneeze.h"
```

## The lifecycle

You implement the [`INSTANCE`](api/INSTANCE.md) lifecycle by defining any of four
free functions. Each has a do-nothing **weak** default in the SDK, so you override
only what you use - the C twin of the Rust trait's default methods. There is no
`instance!` macro to place: linking the SDK provides the seven raw ABI exports
(`Alloc`, `Free`, `Notify`, `Init`, `Open`, `Close`, `Shutdown`) and routes them
to your hooks.

```c
void Instance_Init     (void);                 // module loaded, before any fabric
void Instance_Open     (HFABRIC twFabricIx);   // a fabric opened
void Instance_Close    (HFABRIC twFabricIx);   // a fabric closed
void Instance_Shutdown (void);                 // module unloading
```

A minimal module:

```c
#include "sneeze.h"

void Instance_Open (HFABRIC twFabricIx)
{
   Console_Log (twFabricIx, "hello from wasm");

   HMAPOBJECT pRoot = MapObject_Physical ();
   MapObject_Name      (pRoot, "Stool");
   MapObject_Reference (pRoot, "assets/Stool.glb");

   HNODE qwNode = Scene_Node_Root (twFabricIx, pRoot);
   Node_Scale (qwNode, 2.0);

   MapObject_Free (pRoot);
}
```

Because one WASM instance can serve several fabrics at once, key any per-fabric
state you retain by the `HFABRIC` value (see [`Open`](api/INSTANCE.md#open)).

## Function reference

Semantics live on the linked per-class pages; only the C signatures are given here.

### Console

See [CONSOLE](api/CONSOLE.md).

```c
void Console_Log             (HFABRIC twFabricIx, const char* sText);
void Console_Debug           (HFABRIC twFabricIx, const char* sText);
void Console_Info            (HFABRIC twFabricIx, const char* sText);
void Console_Warn            (HFABRIC twFabricIx, const char* sText);
void Console_Error           (HFABRIC twFabricIx, const char* sText);
void Console_Group           (HFABRIC twFabricIx, const char* sText);
void Console_Group_Collapsed (HFABRIC twFabricIx, const char* sText);
void Console_Count           (HFABRIC twFabricIx, const char* sText);
void Console_Count_Reset     (HFABRIC twFabricIx, const char* sText);
void Console_Time            (HFABRIC twFabricIx, const char* sText);
void Console_Time_End        (HFABRIC twFabricIx, const char* sText);
void Console_Time_Log        (HFABRIC twFabricIx, const char* sText);
void Console_Assert          (HFABRIC twFabricIx, bool bCondition, const char* sText);
void Console_Group_End       (HFABRIC twFabricIx);
```

### Storage

See [STORAGE](api/STORAGE.md). `eScope` is an `enum eSNEEZE_ABI_SILO_SCOPE`.
`Storage_Get` returns a `malloc`'d, NUL-terminated string the caller frees with
`free ()`, or `NULL` for a missing or null value.

```c
bool  Storage_Has    (HFABRIC twFabricIx, enum eSNEEZE_ABI_SILO_SCOPE eScope, const char* sPath);
char* Storage_Get    (HFABRIC twFabricIx, enum eSNEEZE_ABI_SILO_SCOPE eScope, const char* sPath);
bool  Storage_Set    (HFABRIC twFabricIx, enum eSNEEZE_ABI_SILO_SCOPE eScope, const char* sPath, const char* sJson);
bool  Storage_Remove (HFABRIC twFabricIx, enum eSNEEZE_ABI_SILO_SCOPE eScope, const char* sPath);
```

### Data

See [DATA](api/DATA.md). `Data_Get` returns a `malloc`'d, NUL-terminated string
the caller frees with `free ()`, or `NULL` for a missing or null value.

```c
bool  Data_Has (HFABRIC twFabricIx, const char* sPath);
char* Data_Get (HFABRIC twFabricIx, const char* sPath);
```

### Scene

See [SCENE](api/SCENE.md). `Scene_Node_Open` reads the parent from the map
object's own parent index, and `Scene_Node_Close` takes only the node, so both
ignore `twFabricIx` (it is accepted for call-site symmetry with the rest of the
fabric-rooted API).

```c
HNODE Scene_Node_Root  (HFABRIC twFabricIx, HMAPOBJECT pObject);
HNODE Scene_Node_Map   (HFABRIC twFabricIx, const char* sPath);
HNODE Scene_Node_Open  (HFABRIC twFabricIx, HMAPOBJECT pObject);
bool  Scene_Node_Close (HFABRIC twFabricIx, HNODE qwComposed);
```

### Node

See [NODE](api/NODE.md). `Node_Class` / `Node_ObjectIx` / `Node_IsValid` are the
C forms of the `NODE` accessors (`IsValid` is `false` only when the creating call
failed).

```c
void Node_Position   (HNODE qwComposed, double dX, double dY, double dZ);
void Node_Scale      (HNODE qwComposed, double dScale);
void Node_Scale_Axes (HNODE qwComposed, double dX, double dY, double dZ);
void Node_Bound      (HNODE qwComposed, double dX, double dY, double dZ);
void Node_Name       (HNODE qwComposed, const char* sName);
void Node_Resource   (HNODE qwComposed, const char* sUrl);
void Node_Panel      (HNODE qwComposed, const char* sRml);

uint16_t Node_Class    (HNODE qwComposed);
uint64_t Node_ObjectIx (HNODE qwComposed);
bool     Node_IsValid  (HNODE qwComposed);
```

### Map-object builder

See [SNEEZE_ABI_MAPOBJECT](api/MAPOBJECT.md). A class factory (or `MapObject_New`)
allocates a builder; fill it with the setters, hand it to `Scene_Node_Root` /
`Scene_Node_Open`, then release it with `MapObject_Free`. `dwColor` is `0xRRGGBB`,
packed into `fColor`'s bits.

```c
HMAPOBJECT MapObject_New         (uint16_t wClass);
HMAPOBJECT MapObject_Root        (void);
HMAPOBJECT MapObject_Celestial   (void);
HMAPOBJECT MapObject_Terrestrial (void);
HMAPOBJECT MapObject_Physical    (void);
HMAPOBJECT MapObject_Panel       (void);
HMAPOBJECT MapObject_Light       (void);
void       MapObject_Free        (HMAPOBJECT pObject);

void MapObject_Parent     (HMAPOBJECT pObject, uint16_t wClass, uint64_t twObjectIx);
void MapObject_ObjectIx   (HMAPOBJECT pObject, uint64_t twObjectIx);
void MapObject_Name       (HMAPOBJECT pObject, const char* sName);
void MapObject_Reference  (HMAPOBJECT pObject, const char* sReference);
void MapObject_Type       (HMAPOBJECT pObject, uint8_t bType);
void MapObject_Subtype    (HMAPOBJECT pObject, uint8_t bSubtype);
void MapObject_Position   (HMAPOBJECT pObject, double dX, double dY, double dZ);
void MapObject_Rotation   (HMAPOBJECT pObject, double dX, double dY, double dZ, double dW);
void MapObject_Scale      (HMAPOBJECT pObject, double dScale);
void MapObject_Scale_Axes (HMAPOBJECT pObject, double dX, double dY, double dZ);
void MapObject_Bound      (HMAPOBJECT pObject, double dX, double dY, double dZ);
void MapObject_Orbit      (HMAPOBJECT pObject, double dA, double dB, int64_t tmPeriod, int64_t tmOrigin);
void MapObject_Properties_Celestial (HMAPOBJECT pObject, float fMass, float fGravity, uint32_t dwColor, float fBrightness, float fReflectivity);
void MapObject_Properties_Light     (HMAPOBJECT pObject, float fOpeningAngle, float fFalloffAngle, uint32_t dwColor, float fBrightness);
```

### Snapshot views

The [Open snapshot](api/SNAPSHOT.md) is parsed once, privately, before
`Instance_Open` runs. Every getter returns a stable pointer valid until the next
`Open`; a missing field yields an empty string, never `NULL`. All accept an
`HFABRIC` for call-site symmetry; the SDK holds one global snapshot (the
most-recently-opened fabric), mirroring the Rust SDK.

```c
// LOCATION - see api/LOCATION.md
const char* Location_Href     (HFABRIC twFabricIx);
const char* Location_Protocol (HFABRIC twFabricIx);
const char* Location_Host     (HFABRIC twFabricIx);
const char* Location_Pathname (HFABRIC twFabricIx);
const char* Location_Origin   (HFABRIC twFabricIx);

// RESOURCE - see api/RESOURCE.md
uint64_t    Resource_Id   (HFABRIC twFabricIx);
const char* Resource_Name (HFABRIC twFabricIx);

// CONTAINER - see api/CONTAINER.md (Trust is an eSNEEZE_ABI_TRUST value)
const char* Container_Name                (HFABRIC twFabricIx);
const char* Container_Organization        (HFABRIC twFabricIx);
const char* Container_OrganizationHash    (HFABRIC twFabricIx);
const char* Container_Persona             (HFABRIC twFabricIx);
const char* Container_PersonaHash         (HFABRIC twFabricIx);
const char* Container_Fingerprint         (HFABRIC twFabricIx);
int32_t     Container_Trust               (HFABRIC twFabricIx);
const char* Container_DisplayName         (HFABRIC twFabricIx);
const char* Container_DisplayOrganization (HFABRIC twFabricIx);

// SIGNATURE - see api/SIGNATURE.md
const char* Signature_Algorithm      (HFABRIC twFabricIx);
bool        Signature_IsValid        (HFABRIC twFabricIx);
bool        Signature_IsChainTrusted (HFABRIC twFabricIx);
bool        Signature_IsChainExpired (HFABRIC twFabricIx);

// AGENT - see api/AGENT.md
const char* Agent_Browser_Name    (HFABRIC twFabricIx);
const char* Agent_Browser_Version (HFABRIC twFabricIx);
const char* Agent_Engine_Name     (HFABRIC twFabricIx);
const char* Agent_Engine_Version  (HFABRIC twFabricIx);
const char* Agent_Platform        (HFABRIC twFabricIx);
const char* Agent_Language        (HFABRIC twFabricIx);

// SERVICE - see api/SERVICE.md (indexed lists, so a count plus per-index getters)
int32_t     Services_Count        (HFABRIC twFabricIx);
const char* Service_Name          (HFABRIC twFabricIx, int32_t nIz);
const char* Service_Type          (HFABRIC twFabricIx, int32_t nIz);
const char* Service_Endpoint      (HFABRIC twFabricIx, int32_t nIz);
int32_t     Service_Modules_Count (HFABRIC twFabricIx, int32_t nIz);
const char* Service_Module        (HFABRIC twFabricIx, int32_t nIzService, int32_t nIzModule);

// MODULE - see api/MODULE.md
int32_t     Modules_Count (HFABRIC twFabricIx);
const char* Module_Url    (HFABRIC twFabricIx, int32_t nIz);
const char* Module_Hash   (HFABRIC twFabricIx, int32_t nIz);
```

## Memory and strings

- **The `Alloc` / `Free` handshake is handled for you.** The SDK's exports back
  the [Open snapshot](api/SNAPSHOT.md) handshake with `malloc` / `free`; you never
  write those exports. See [Incorporating the ABI](incorporating-the-abi.md#the-open-handshake).
- **`Storage_Get` and `Data_Get` return owned strings.** Free the non-`NULL`
  result with `free ()`. They do the ABI's size-probe-then-exact-reread internally,
  the same as the Rust `Get` returning an owned `String`.
- **Snapshot getters return borrowed strings.** The pointer is owned by the SDK
  and valid until the next `Open`; do not free it. Copy it if you need it longer.
- **Builders are explicit.** `MapObject_*` factories allocate; pair each with a
  `MapObject_Free` once the node is created.

## Building with Emscripten

Your module and the SDK sources compile together into one reactor `.wasm`. From an
activated [emsdk](https://emscripten.org):

```sh
make SNEEZE_ABI_INCLUDE=/path/to/SneezeSDK/include
```

or directly:

```sh
emcc -std=c11 -Os -Wno-address-of-packed-member -Iinclude -Isrc -I../include \
     -sSTANDALONE_WASM -sWASM_BIGINT --no-entry \
     -sERROR_ON_UNDEFINED_SYMBOLS=0 -sMALLOC=emmalloc \
     src/sneeze_ffi.c src/sneeze_json.c src/sneeze_snapshot.c \
     src/sneeze_mapobject.c src/sneeze_objects.c src/sneeze_instance.c \
     example/module.c -o module.wasm
```

- `-sWASM_BIGINT` keeps the ABI's `i64` values (fabric handles, node indices, the
  `Call` return) native across the boundary instead of splitting them.
- `--no-entry` / `-sSTANDALONE_WASM` build a reactor module with no `main`; the
  engine drives it through the exported lifecycle symbols.
- `-sERROR_ON_UNDEFINED_SYMBOLS=0` leaves `Sneeze.Call` as an import for the host.

The result imports only `Sneeze.Call` and exports the seven ABI symbols. Confirm
with `wasm-objdump -x module.wasm`.

## See also

- [Incorporating the ABI](incorporating-the-abi.md) - the wire format beneath the
  SDK, the import/exports, and the `Open` handshake.
- [API overview](api/overview.md) - the object model these functions expose.
- `sdk/include/sneeze_abi.h` - the normative contract, including the flat C API
  name reference this binding follows.
