# SneezeCS

Sneeze C# API - the guest-side .NET SDK for Sneeze WASM modules, published as
`wasi-wasm` via Native AOT (same ABI surface as the Rust and C SDKs).

---

## Documentation

This README covers building the **csstool** example. API semantics match the
[SneezeSDK docs](../docs/README.md) and the per-class pages under
`SneezeSDK/docs/api/` (same fabric, snapshot, and lifecycle model as Rust/C).

---

## Prerequisites

You need the following installed before building. Open a terminal and check each one:

| Tool | Purpose | Check command | Minimum version |
|------|---------|---------------|-----------------|
| **Git** | Clones this repo and examples | `git --version` | any |
| **.NET SDK** | Restores, builds, and publishes `wasi-wasm` Native AOT output | `dotnet --version` | 9.0 or later (use the latest stable SDK from the download link below) |
| **Visual Studio** (optional) | IDE integration for .NET / Native AOT | Open **About Microsoft Visual Studio** | **2026** recommended for full WASI Native AOT in the IDE; **2022** is fine if you publish with **`dotnet`** CLI only |

If `git --version` and `dotnet --version` both print a version, skip ahead to
[10.0 Building csstool](#100-building-csstool). Otherwise, install what is missing:

---

### Git

- **Windows:** Download from [git-scm.com](https://git-scm.com/). Accept defaults. When asked about PATH, choose **Git from the command line and also from 3rd-party software.**
- **Linux:** `sudo apt install git` (Debian/Ubuntu) or `sudo dnf install git` (Fedora)
- **macOS:** `xcode-select --install`

---

### .NET SDK

The SDK drives restore, publish, and the Native AOT `wasi-wasm` cross-compile. You
do **not** need Visual Studio if you use the CLI only.

- **All platforms:** Download the latest **.NET SDK** (not just the runtime) from
  [dotnet.microsoft.com/download](https://dotnet.microsoft.com/download).
- **Windows (optional):** `winget install Microsoft.DotNet.SDK.9` (or the current
  major SDK package shown on the download page).

After installing, close and reopen your terminal so `dotnet` is on PATH.

---

### Visual Studio (optional)

Use this only if you want IDE debugging and project templates. Command-line publish
works with the .NET SDK alone.

- **Visual Studio 2026:** [visualstudio.microsoft.com](https://visualstudio.microsoft.com/) -
  install the workload that includes **.NET** and **WebAssembly / Native AOT** support
  as offered for your edition.
- **Visual Studio 2022:** [Visual Studio 2022 downloads](https://visualstudio.microsoft.com/vs/) -
  Community or higher with **.NET** workloads if you already use VS 2022.

If you do **not** want to upgrade Visual Studio beyond 2022, use **.NET SDK +
`dotnet publish`** ([10.0](#100-building-csstool)) instead of building only inside
the VS 2022 IDE.

---

## 10.0 Building csstool

From a shell with `dotnet` on PATH (see [Prerequisites](#prerequisites)):

```bat
dotnet publish "C:\Dev\OMB\SneezeSDK\CS\examples\csstool\csstool.csproj" -r wasi-wasm -c Release -p:DebugType=none -p:IlcLlvmTarget=wasm32-unknown-wasip1
```

When your clone lives under `RP1`:

```bat
dotnet publish "C:\Dev\RP1\SneezeSDK\CS\examples\csstool\csstool.csproj" -r wasi-wasm -c Release -p:DebugType=none -p:IlcLlvmTarget=wasm32-unknown-wasip1
```

Copy the published `.wasm` into your fabric's `wasm/` folder, point `cwasm.json`
`Modules` at that URL, and deploy `assets/Stool.glb` beside the manifest the same
way as the C **cstool** example.
