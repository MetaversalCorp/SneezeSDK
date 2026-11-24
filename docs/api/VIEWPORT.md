# VIEWPORT

Pointer input the engine pushes to a module. This is a host-to-guest [`Notify`](../incorporating-the-abi.md), not a view you call through [`HOST`](HOST.md). Camera position and rotation (`POSITION_GET`, `POSITION_SET`, `ROTATION_GET`, `ROTATION_SET`, methods 1-4) are reserved in `sneeze_abi.h` and are not implemented. The live method is the panel click below.

## Pointer

When a pointer button releases over a [`PANEL`](NODE.md#panel) node, the engine hit-tests that panel's RML document. If the element under the pointer has an `id`, or an ancestor of it does, the engine reports the nearest such id. A release that hits no identified element produces no notify. The packet carries the fabric and that id. It does not carry the node, the pointer coordinates, or which button.

Delivery is asynchronous. The compositor records the click, finishes the frame, and then calls your `Notify` export. Every module instance in the store receives the packet; keep the one whose `twFabricIx` is the fabric you have open and ignore the rest.

The packet is the usual 8-byte header followed by this payload:

| Field | Type | Meaning |
|-------|------|---------|
| `wType` | `u16` | `kSNEEZE_ABI_TYPE_VIEWPORT` (5) |
| `wMethod` | `u16` | `kSNEEZE_ABI_METHOD_VIEWPORT_POINTER` (5) |
| `twFabricIx` | `u64` | The fabric whose panel was clicked. |
| `nIdBytes` | `u64` | Byte length of the id that follows. The host caps this at 128 and truncates a longer id. |
| id | `u8[nIdBytes]` | The element id as UTF-8, with no trailing NUL. |

`twFabricIx` and `nIdBytes` are little-endian and sit immediately after the header. The id bytes follow them with no padding.

The `id` is the Rml element id nearest the click, walking from the event target toward the document root. Give each clickable control an `id` in the RML you pass to [`NODE::Panel`](NODE.md#panel); that string is what comes back. A click on a child that has no id of its own reports the ancestor's id.

The truncation is by byte count. An id longer than 128 bytes can be cut in the middle of a multi-byte UTF-8 character, so keep ids short and ASCII.

The language bindings dispatch timer, request, socket, and Socket.IO events from their generated `Notify`. They have no hook for this packet and drop it, so read method 5 in your own `Notify`.

```xml
<button id="reset">Reset</button>
```

A click on that button delivers the UTF-8 bytes of `reset` and `nIdBytes` equal to 5.

## See also

- [`NODE::Panel`](NODE.md#panel) - the RML document whose element ids this notify reports.
- [Incorporating the ABI](../incorporating-the-abi.md) - the `Notify` export and the packet header.
- [API overview](overview.md) - which event kinds `Notify` delivers.
