# Quickstart: Your First Sneeze WASM Module

This guide takes you from nothing to a running `.wasm` module. Pick your language — **Rust** or **C** (built with Emscripten) — and follow the matching track.

> **Platform note:** The shell commands in this guide are written for **Linux and macOS**. Windows users: see the [Windows quickstart](quickstart-windows.md) for equivalent commands.

---

## Step 1 — Clone the repository

```sh
git clone --recurse-submodules https://github.com/MetaversalCorp/SneezeSDK.git
cd SneezeSDK
```

> If you don't have Git, install it from your package manager (e.g. `sudo apt install git` or `brew install git`).
>
> **Already cloned without `--recurse-submodules`?** Run this from inside the repo to populate the submodules now:
> ```sh
> git submodule update --init --recursive
> ```

After cloning, your directory layout will look like this:

```
~/dev/
└── SneezeSDK/                  ← you are here after `cd SneezeSDK`
    ├── sdk/
    │   └── include/
    │       └── sneeze_abi.h    ← canonical ABI header
    ├── Rust/                   ← SneezeSDK_Rust submodule (used by Track A)
    └── C/                      ← SneezeSDK_C submodule (used by Track B)
```

Your own module projects will be created **inside** `SneezeSDK/` in the steps below, so the relative paths to the submodules stay simple.

---

## Track A — Rust

### Step 2A — Install Rust and the WASM target

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

### Step 3A — Create your project

From inside `SneezeSDK/`, create your module as a sibling of the `Rust/` submodule:

```sh
# pwd: ~/dev/SneezeSDK
cargo new --lib my_module
cd my_module
```

Your layout is now:

```
SneezeSDK/
├── Rust/           ← SneezeSDK_Rust (local library source)
├── C/
├── sdk/
└── my_module/      ← your new project (you are here)
    ├── Cargo.toml
    └── src/
        └── lib.rs
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
sneeze = { path = "../Rust" }

[profile.release]
opt-level = "s"
lto = true
```

> `path = "../Rust"` points at the `SneezeSDK/Rust/` submodule you cloned in Step 1 — no internet access needed at build time.

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

> `lib.rs` is Cargo's default entry point for a library crate — the `--lib` flag you passed to `cargo new` is what makes this a library rather than an executable.

The `instance!` macro wires up all seven ABI exports for you.

### Step 4A — Build

```sh
# pwd: ~/dev/SneezeSDK/my_module
cargo build --target wasm32-unknown-unknown --release
```

Your module is at `target/wasm32-unknown-unknown/release/my_module.wasm`.

---

## Track B — C with Emscripten

### Step 2B — Install Emscripten

Clone and set up emsdk **alongside** (not inside) `SneezeSDK/`:

```sh
# pwd: ~/dev  (the same parent directory that contains SneezeSDK/)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # adds emcc to your PATH — run this every new shell
```

Your layout is now:

```
~/dev/
├── SneezeSDK/
│   ├── sdk/include/
│   ├── Rust/
│   └── C/              ← SneezeSDK_C (SDK source files are here)
└── emsdk/              ← Emscripten toolchain
```

Verify:

```sh
emcc --version
```

> You must `source ./emsdk_env.sh` at the start of every new shell session before using `emcc`.

### Step 3B — Write your module

Create your `.c` file **inside `SneezeSDK/C/`**, alongside the SDK source files that `emcc` will compile:

```sh
# pwd: ~/dev/SneezeSDK/C
```

Create `my_module.c` here:

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

### Step 4B — Build

Run from inside `SneezeSDK/C/` (with `emsdk_env.sh` already sourced):

```sh
# pwd: ~/dev/SneezeSDK/C
make SNEEZE_ABI_INCLUDE=../sdk/include
```

Or invoke `emcc` directly:

```sh
# pwd: ~/dev/SneezeSDK/C
emcc -std=c11 -Os -Wno-address-of-packed-member \
     -Iinclude -Isrc -I../sdk/include \
     -sSTANDALONE_WASM -sWASM_BIGINT --no-entry \
     -sERROR_ON_UNDEFINED_SYMBOLS=0 -sMALLOC=emmalloc \
     src/sneeze_ffi.c src/sneeze_json.c src/sneeze_snapshot.c \
     src/sneeze_mapobject.c src/sneeze_objects.c src/sneeze_instance.c \
     my_module.c -o my_module.wasm
```

> `-I../sdk/include` resolves to `SneezeSDK/sdk/include/` — the ABI header shared by all languages.

You should get a `my_module.wasm` file. Confirm it has the right shape:

```sh
wasm-objdump -x my_module.wasm
```

It should import only `Sneeze.Call` and export the seven ABI symbols (`Alloc`, `Free`, `Notify`, `Init`, `Open`, `Close`, `Shutdown`).

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
