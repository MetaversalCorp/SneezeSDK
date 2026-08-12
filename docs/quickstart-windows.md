# Quickstart (Windows): Your First Sneeze WASM Module

This guide takes you from nothing to a running `.wasm` module on Windows. Pick your language — **Rust** or **C** (built with Emscripten) — and follow the matching track.

> **Platform note:** This guide is for **Windows**. Linux and macOS users: see the [standard quickstart](quickstart.md).

### Which terminal to use

- **Track A (Rust):** Use **PowerShell**. `cargo`, `rustup`, and `winget` all work natively in PowerShell with no extra setup.
- **Track B (C/Emscripten):** Use **PowerShell** (Windows Terminal with the PowerShell tab). The Emscripten environment script is a `.bat` file that works in both `cmd` and PowerShell; PowerShell is recommended for consistency.

Open Windows Terminal and select a **PowerShell** tab before starting either track.

---

## Step 1 — Clone the repository

```powershell
git clone --recurse-submodules https://github.com/MetaversalCorp/SneezeSDK.git
cd SneezeSDK
```

> If you don't have Git, install it from [git-scm.com](https://git-scm.com/download/win) or run `winget install Git.Git`.
>
> **Already cloned without `--recurse-submodules`?** Run this from inside the repo to populate the submodules now:
> ```powershell
> git submodule update --init --recursive
> ```

After cloning, your directory layout will look like this:

```
C:\dev\
└── SneezeSDK\                  ← you are here after `cd SneezeSDK`
    ├── sdk\
    │   └── include\
    │       └── sneeze_abi.h    ← canonical ABI header
    ├── Rust\                   ← SneezeSDK_Rust submodule (used by Track A)
    └── C\                      ← SneezeSDK_C submodule (used by Track B)
```

Your own module projects will be created **inside** `SneezeSDK\` in the steps below, so the relative paths to the submodules stay simple.

---

## Track A — Rust

### Step 2A — Install Rust and the WASM target

Install Rust using `winget` in PowerShell:

```powershell
winget install Rustlang.Rustup
```

Then close and reopen Windows Terminal (so the updated `PATH` takes effect), and add the WebAssembly target:

```powershell
rustup target add wasm32-unknown-unknown
```

Verify:

```powershell
cargo --version
rustc --version
```

> Alternatively, download and run the `rustup-init.exe` installer directly from [rustup.rs](https://rustup.rs).

### Step 3A — Create your project

From inside `SneezeSDK\`, create your module as a sibling of the `Rust\` submodule:

```powershell
# pwd: C:\dev\SneezeSDK
cargo new --lib my_module
cd my_module
```

Your layout is now:

```
SneezeSDK\
├── Rust\           ← SneezeSDK_Rust (local library source)
├── C\
├── sdk\
└── my_module\      ← your new project (you are here)
    ├── Cargo.toml
    └── src\
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

> `path = "../Rust"` points at the `SneezeSDK\Rust\` submodule you cloned in Step 1 — no internet access needed at build time.

Replace `src\lib.rs` with:

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

### Step 4A — Build

```powershell
# pwd: C:\dev\SneezeSDK\my_module
cargo build --target wasm32-unknown-unknown --release
```

Your module is at `target\wasm32-unknown-unknown\release\my_module.wasm`.

---

## Track B — C with Emscripten

### Step 2B — Install Emscripten

Clone and set up emsdk **alongside** (not inside) `SneezeSDK\`:

```powershell
# pwd: C:\dev  (the same parent directory that contains SneezeSDK\)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
.\emsdk install latest
.\emsdk activate latest
.\emsdk_env.bat     # adds emcc to your PATH for this session — run this every new terminal
```

Your layout is now:

```
C:\dev\
├── SneezeSDK\
│   ├── sdk\include\
│   ├── Rust\
│   └── C\              ← SneezeSDK_C (SDK source files are here)
└── emsdk\              ← Emscripten toolchain
```

Verify:

```powershell
emcc --version
```

> You must run `.\emsdk_env.bat` at the start of every new PowerShell session before using `emcc`.

### Step 3B — Write your module

Create your `.c` file **inside `SneezeSDK\C\`**, alongside the SDK source files that `emcc` will compile:

```powershell
# pwd: C:\dev\SneezeSDK\C
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

`make` is not available by default on Windows, so invoke `emcc` directly. Run from inside `SneezeSDK\C\` (with `emsdk_env.bat` already run), using PowerShell's backtick (`` ` ``) for line continuation:

```powershell
# pwd: C:\dev\SneezeSDK\C
emcc -std=c11 -Os -Wno-address-of-packed-member `
     -Iinclude -Isrc -I..\sdk\include `
     -sSTANDALONE_WASM -sWASM_BIGINT --no-entry `
     -sERROR_ON_UNDEFINED_SYMBOLS=0 -sMALLOC=emmalloc `
     src/sneeze_ffi.c src/sneeze_json.c src/sneeze_snapshot.c `
     src/sneeze_mapobject.c src/sneeze_objects.c src/sneeze_instance.c `
     my_module.c -o my_module.wasm
```

> `-I..\sdk\include` resolves to `SneezeSDK\sdk\include\` — the ABI header shared by all languages.

You should get a `my_module.wasm` file. Confirm it has the right shape:

```powershell
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
