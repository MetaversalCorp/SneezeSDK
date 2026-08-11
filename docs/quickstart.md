# Quickstart: Your First Sneeze WASM Module

This guide takes you from nothing to a running `.wasm` module. Pick your language — **C** (built with Emscripten) or **Rust** — and follow the matching track.

> **Platform note:** The shell commands in this guide are written for **Linux and macOS**. Windows users: see the [Windows quickstart](quickstart-windows.md) for equivalent commands.

---

## Step 1 — Clone the repository

```sh
git clone https://github.com/MetaversalCorp/SneezeSDK.git
cd SneezeSDK
```

The repository contains:
- `sdk/include/sneeze_abi.h` — the canonical ABI header (all languages share it).
- `Rust/` — the Rust SDK crate (git submodule → [SneezeSDK_Rust](https://github.com/MetaversalCorp/SneezeSDK_Rust)).
- The C SDK sources (in `SneezeSDK_C`, referenced as a sibling or submodule).

---

## Track A — C with Emscripten

### Step 2A — Install Emscripten

Follow the [emsdk Getting Started guide](https://emscripten.org/docs/getting_started/downloads.html):

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # add emcc to your PATH (run this every new shell)
```

Verify:

```sh
emcc --version
```

### Step 3A — Write your module

Create a file `my_module.c`:

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

You only define the lifecycle hooks you need. The SDK supplies do-nothing defaults for `Instance_Init`, `Instance_Close`, and `Instance_Shutdown`, so you can omit them here.

### Step 4A — Build

From the root of the `SneezeSDK_C` directory (with `emsdk_env.sh` already sourced):

```sh
make SNEEZE_ABI_INCLUDE=/path/to/SneezeSDK/sdk/include
```

Or invoke `emcc` directly:

```sh
emcc -std=c11 -Os -Wno-address-of-packed-member \
     -Iinclude -Isrc -I/path/to/SneezeSDK/sdk/include \
     -sSTANDALONE_WASM -sWASM_BIGINT --no-entry \
     -sERROR_ON_UNDEFINED_SYMBOLS=0 -sMALLOC=emmalloc \
     src/sneeze_ffi.c src/sneeze_json.c src/sneeze_snapshot.c \
     src/sneeze_mapobject.c src/sneeze_objects.c src/sneeze_instance.c \
     my_module.c -o my_module.wasm
```

You should get a `my_module.wasm` file. Confirm it has the right shape:

```sh
wasm-objdump -x my_module.wasm
```

It should import only `Sneeze.Call` and export the seven ABI symbols (`Alloc`, `Free`, `Notify`, `Init`, `Open`, `Close`, `Shutdown`).

---

## Track B — Rust

### Step 2B — Install Rust and the WASM target

Install Rust via [rustup](https://rustup.rs):

```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

Add the WebAssembly target:

```sh
rustup target add wasm32-unknown-unknown
```

Verify:

```sh
cargo --version
rustc --version
```

### Step 3B — Create your project

```sh
cargo new --lib my_module
cd my_module
```

Edit `Cargo.toml`:

```toml
[package]
name = "my_module"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib"]

[dependencies]
sneeze = { git = "https://github.com/MetaversalCorp/SneezeSDK_Rust.git" }

[profile.release]
opt-level = "s"
lto = true
```

Replace `src/lib.rs` with:

```rust
use sneeze::*;

struct MY_MODULE;

impl INSTANCE for MY_MODULE
{
    fn Open (pHost: &HOST)
    {
        pHost.Console ().Log ("hello from wasm");

        let mut pRoot = SNEEZE_ABI_MAPOBJECT::Physical ();
        pRoot.Name ("Stool").Reference ("assets/Stool.glb");
        pHost.Fabric ().Node_Root (&pRoot);
    }
}

sneeze::instance! (MY_MODULE);
```

The `instance!` macro wires up all seven ABI exports for you.

### Step 4B — Build

```sh
cargo build --target wasm32-unknown-unknown --release
```

Your module is at `target/wasm32-unknown-unknown/release/my_module.wasm`.

---

## What you built

Both tracks produce a reactor `.wasm` that:
- Imports one function: `Sneeze.Call` (provided by the engine at runtime).
- Exports the seven lifecycle symbols the engine calls (`Init`, `Open`, `Close`, `Shutdown`, `Alloc`, `Free`, `Notify`).
- Logs `"hello from wasm"` to the developer console when a fabric opens, then adds a scaled physical node to the scene.

---

## Next steps

- **[API overview](api/overview.md)** — understand the object model: `HOST`, fabrics, nodes, the `Open` lifecycle.
- **[The C SDK](c-sdk.md)** — full C function reference.
- **[Incorporating the ABI](incorporating-the-abi.md)** — the wire format underneath the SDK, for deeper understanding or porting to a new language.
- **Per-class pages** ([CONSOLE](api/CONSOLE.md), [SCENE](api/SCENE.md), [NODE](api/NODE.md), etc.) — every method documented with parameters, return values, and examples.
