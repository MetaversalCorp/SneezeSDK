# SneezeSDK

Collection of Sneeze WASM SDKs. A Sneeze module is a small, sandboxed
WebAssembly program that a spatial fabric loads to build and drive its scene. A
module talks to the engine across one narrow, frozen interface (the ABI); each
per-language SDK wraps that interface in ordinary typed objects so you write
against the same object model, `Open` lifecycle, and method semantics in every
language.

This repository is the umbrella: each language SDK is a git submodule, and the
shared documentation and the canonical ABI header live here at the root.

## Supported languages

| Language | Folder | Repository | Toolchain | Status |
|----------|--------|------------|-----------|--------|
| **Rust** | `Rust/` | `SneezeSDK_Rust` | Cargo, `wasm32` target | Reference SDK |
| **C** | `C/` | `SneezeSDK_C` | Emscripten (`emcc`) | Available |
| **C++** | `Cpp/` | `SneezeSDK_CPP` | Emscripten (`emcc`) | Available |
| **C#** | `CS/` | `SneezeSDK_CS` | .NET SDK, `wasi-wasm` Native AOT | Available |
| **AssemblyScript** | `AS/` | `SneezeSDK_AS` | AssemblyScript, `asc` | Available |

### Rust

The reference SDK and the canonical shape every other binding mirrors. You
implement the `INSTANCE` trait (`Init`, `Open`, `Close`, `Shutdown`) and wire it
up with the `instance!` macro, then reach every subsystem through the `FABRIC`
root. Distributed as a Cargo crate and compiled to a `wasm32` reactor module. See [RUST/README.md](https://github.com/MetaversalCorp/SneezeSDK_Rust) for building the
`csstool` example.

### C

The C binding, built with Emscripten. C has no methods, so the SDK is flat:
every subsystem method becomes a free function whose first argument is the handle
Rust would have called the method on (for example `pFabric.Console ().Log (s)`
becomes `Console_Log (twFabricIx, s)`). It layers directly on the canonical ABI
header and mirrors the Rust SDK function for function. See [C/README.md](https://github.com/MetaversalCorp/SneezeSDK_C) for building the
`cstool` example.

### C++

The C++ binding over the same frozen ABI, a sibling to the C and Rust SDKs. Built
with Emscripten and following the shared object model and `Open` lifecycle
documented for every language. See [CPP/README.md](https://github.com/MetaversalCorp/SneezeSDK_CPP) for building the
`cppstool` example.

### C#

The guest-side .NET SDK, published as a `wasi-wasm` reactor module via Native AOT
(same ABI surface as the Rust and C SDKs). Build with the .NET SDK 9.0 or later;
Visual Studio is optional. See [CS/README.md](https://github.com/MetaversalCorp/SneezeSDK_CS) for building the
`csstool` example.

### AssemblyScript

The guest-side AssemblyScript SDK, built with `asc`. It is the TypeScript-syntax
binding over the same canonical ABI, mirroring the Rust SDK class for class
alongside the C, C++, and C# ports. See [AS/README.md](https://github.com/MetaversalCorp/SneezeSDK_AS) for building
the `as_stool` example.

## Versioning

The root `VERSION` file carries the version of the collection as a whole. It
follows the language SDKs: the major and minor numbers are shared, and the patch
number is the highest patch number found across the SDKs, so the umbrella never
lags behind its newest member.

```
SneezeSDK        0.1.Y   where Y = MAX (A, B, C, D, E)

SneezeSDK_AS     0.1.A
SneezeSDK_C      0.1.B
SneezeSDK_CPP    0.1.C
SneezeSDK_CS     0.1.D
SneezeSDK_Rust   0.1.E
```

**A compatible version file is needed for all the different versions of the
SneezeSDK.** Every SDK therefore declares its version the same way: a `VERSION`
file at the root of the SDK holding a single `MAJOR.MINOR.PATCH` line. Same
filename, same location, same format in every language, so the umbrella reads one
kind of file instead of five toolchain-specific manifests.

| SDK | Version file | Toolchain mirror |
|-----|--------------|------------------|
| `AS/` | `AS/VERSION` | `package.json` |
| `C/` | `C/VERSION` | - |
| `Cpp/` | `Cpp/VERSION` | - |
| `CS/` | `CS/VERSION` | - |
| `Rust/` | `Rust/VERSION` | `Cargo.toml` |

Where a toolchain needs the version in its own manifest as well (Cargo for Rust,
npm for AssemblyScript), the `VERSION` file is authoritative and the manifest
mirrors it, so the two must be bumped together.

When an SDK's version changes, the root `VERSION` is updated to the new highest
patch number. Releases are tagged `vMAJOR.MINOR.PATCH` to match the `VERSION`
file.

## Shared resources at the root

- `VERSION` - the version of the collection, `0.1.Y` as described above.
- `include/sneeze_abi.h` - the canonical, frozen ABI header. The normative
  contract every SDK layers on, including the flat C API name reference.
- `docs/` - language-neutral documentation: the [SDK reference](docs/README.md),
  the [API overview](docs/api/overview.md), the per-class API pages under
  `docs/api/`, [Incorporating the ABI](docs/incorporating-the-abi.md), and
  [The C SDK](docs/c-sdk.md).

## Getting the submodules

The language SDKs are git submodules. Clone with them included, or initialize
them after cloning:

```sh
git clone --recurse-submodules <repo-url>

# or, if already cloned:
git submodule update --init --recursive
```
