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
| **C#** | `CS/` | (in-tree) | .NET SDK, `wasi-wasm` Native AOT | Available |

### Rust

The reference SDK and the canonical shape every other binding mirrors. You
implement the `INSTANCE` trait (`Init`, `Open`, `Close`, `Shutdown`) and wire it
up with the `instance!` macro, then reach every subsystem through the `FABRIC`
root. Distributed as a Cargo crate and compiled to a `wasm32` reactor module.

### C

The C binding, built with Emscripten. C has no methods, so the SDK is flat:
every subsystem method becomes a free function whose first argument is the handle
Rust would have called the method on (for example `pFabric.Console ().Log (s)`
becomes `Console_Log (twFabricIx, s)`). It layers directly on the canonical ABI
header and mirrors the Rust SDK function for function. See
[The C SDK](docs/c-sdk.md).

### C++

The C++ binding over the same frozen ABI, a sibling to the C and Rust SDKs. Built
with Emscripten and following the shared object model and `Open` lifecycle
documented for every language.

### C#

The guest-side .NET SDK, published as a `wasi-wasm` reactor module via Native AOT
(same ABI surface as the Rust and C SDKs). Build with the .NET SDK 9.0 or later;
Visual Studio is optional. See [CS/README.md](CS/README.md) for building the
`csstool` example.

## Shared resources at the root

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
