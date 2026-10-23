<p align="center">
  <img src="assets/branding/obsidian-readme-banner.png" alt="OBSIDIAN" width="100%">
</p>

<p align="center">
  <strong>OFFENSIVE PE PACKING RESEARCH FOR WINDOWS x86-64</strong>
</p>

<p align="center">
  <a href="#build"><img src="https://img.shields.io/badge/BUILD-C++17-C4C9D0?style=for-the-badge&labelColor=050607" alt="C++17"></a>
  <a href="#requirements"><img src="https://img.shields.io/badge/TARGET-WINDOWS_x64-C4C9D0?style=for-the-badge&labelColor=050607" alt="Windows x64"></a>
  <a href="https://github.com/Zibgame/Obsidian-Packer/releases"><img src="https://img.shields.io/github/v/release/Zibgame/Obsidian-Packer?include_prereleases&style=for-the-badge&labelColor=050607&color=7C2CFF" alt="Latest release"></a>
  <a href="#current-state"><img src="https://img.shields.io/badge/STATUS-RESEARCH_BUILD-7C2CFF?style=for-the-badge&labelColor=050607" alt="Research build"></a>
  <a href="#license"><img src="https://img.shields.io/badge/LICENSE-NOT_SELECTED-555B66?style=for-the-badge&labelColor=050607" alt="No license"></a>
</p>

<p align="center">
  <code>COMPRESS</code> · <code>RESTORE</code> · <code>RELOCATE</code> · <code>RESOLVE</code> · <code>EXECUTE</code>
</p>

---

OBSIDIAN is an open-source **PE32+ packer and in-memory loader**. It takes a Windows x86-64 executable, compresses it into a custom runtime stub, rebuilds the original image in memory and transfers execution to its real entry point.

It exists for offensive-security research: custom loader development, packer fingerprinting, static-signature mutation and controlled antivirus testing. The goal is not disguised as compression, software protection or optimization.

> **This project studies anti-detection. That is the point.**<br>
> It does not promise FUD, permanent invisibility or a universal antivirus bypass. Those claims are bullshit. Detection is measurable, temporary and adversarial.

> [!CAUTION]
> OBSIDIAN implements techniques also used by malware loaders. Use it only on binaries and systems you own or are explicitly authorized to test. Offensive capability is not authorization.

## What it does

```text
                         ┌────────────────────┐
                         │   obsidian-stub    │
                         └─────────┬──────────┘
                                   │ inject .obsram
                                   ▼
┌──────────────┐   validate   ┌──────────────┐   append payload   ┌──────────────┐
│  payload.exe │ ───────────► │   obsidian   │ ────────────────► │  output.exe  │
└──────────────┘   compress   └──────────────┘                    └──────┬───────┘
                                                                        │ run
                                                                        ▼
                                                              restore PE in memory
                                                              resolve + relocate
                                                              execute entry point
```

| Packing | Runtime |
| --- | --- |
| Validates PE32+ / AMD64 input | Reads its own embedded payload |
| Compresses with XPRESS_HUFF | Rebuilds headers and sections in `.obsram` |
| Adds the `.obsram` memory area | Applies `DIR64` relocations |
| Appends payload and `OBSD` footer | Resolves imports by name or ordinal |
| Produces a standalone `output.exe` | Restores TLS, permissions and execution |

The original executable is restored directly in memory. No temporary copy of the unpacked PE is written to disk.

## Quick start

### Requirements

- Windows x86-64
- MinGW-w64 with C++17 support
- GNU Make
- Windows Compression API
- Cabinet import library (`-lcabinet`)

### Build

```console
mingw32-make parallel
mingw32-make examples
```

This creates:

```text
obsidian.exe
obsidian-stub.exe
examples/hello.exe
examples/args_echo.exe
examples/tls_demo.exe
```

### Pack

```console
.\obsidian.exe .\examples\hello.exe
```

The default stub is detected automatically and the packed binary is written to `output.exe`.

Use a custom stub explicitly when needed:

```console
.\obsidian.exe .\path\to\obsidian-stub.exe .\payload.exe
```

### Execute

```console
.\output.exe
echo %ERRORLEVEL%
```

The `hello.exe` fixture prints its message and returns exit code `42`.

Arguments are passed through the current process environment:

```console
.\obsidian.exe .\examples\args_echo.exe
.\output.exe alpha beta gamma
```

## Current state

OBSIDIAN is a **working execution prototype**, not a finished universal packer.

| Capability | State |
| --- | :---: |
| PE32+ / AMD64 validation | **Working** |
| XPRESS_HUFF compression | **Working** |
| Runtime `.obsram` injection | **Working** |
| In-memory PE reconstruction | **Working** |
| `IMAGE_REL_BASED_DIR64` relocations | **Working** |
| Import resolution | **Working** |
| x64 exception table registration | **Working** |
| Main-thread static TLS | **Working** |
| TLS callbacks | **Working** |
| Section memory protections | **Working** |
| Original entry-point transfer | **Working** |
| Argument and exit-code propagation | **Working** |
| Resource registration | **Not supported** |
| Full TLS behavior for new threads | **Not supported** |
| Mutation engine | **Planned** |
| Detection benchmarking | **Planned** |

End-to-end execution has been validated on the fixtures shipped with this repository. Compatibility with arbitrary third-party software is not claimed.

## Runtime internals

<details>
<summary><strong>Open the loader pipeline</strong></summary>

When `output.exe` starts, the stub:

1. opens its own executable and reads the final 20-byte footer;
2. extracts and decompresses the embedded original PE;
3. locates the reserved `.obsram` section;
4. copies PE headers and sections into memory layout;
5. applies base relocations for the real `.obsram` address;
6. loads imported DLLs and patches the original IAT;
7. registers `.pdata` through `RtlAddFunctionTable`;
8. allocates static TLS for the main thread;
9. rebuilds page protections from section characteristics;
10. validates the restored image and entry point;
11. calls TLS callbacks with `DLL_PROCESS_ATTACH`;
12. transfers execution to the original entry point.

The packed file layout is deliberately small:

```text
┌──────────────────────────────────────────┐
│ Modified PE runtime stub                 │
│ └── .obsram: restored image memory       │
├──────────────────────────────────────────┤
│ XPRESS_HUFF compressed original PE       │
├────────┬────────────────┬────────────────┤
│ OBSD   │ packed size    │ original size  │
│ 4 B    │ uint64 LE      │ uint64 LE      │
└────────┴────────────────┴────────────────┘
```

</details>

<details>
<summary><strong>Open the source layout</strong></summary>

```text
Obsidian-Packer/
├── assets/branding/       Brand assets
├── docs/                  Runtime notes
├── examples/              PE test fixtures
├── include/
│   ├── compression/       Compression interface
│   ├── file/              Binary file helpers
│   └── pe/                PE parser interface
├── src/
│   ├── compression/       XPRESS_HUFF implementation
│   ├── file/              Binary I/O
│   ├── pe/                PE parsing and section injection
│   ├── stub/              In-memory runtime loader
│   └── main.cpp           Packer entry point
└── Makefile
```

</details>

## Offensive research direction

Public packers become signatures of their own. Fixed section names, repeated metadata, known decompression loops, stable stubs and predictable file layouts give defenders something reliable to match.

OBSIDIAN is being built to attack that stability.

The research objective is to generate controlled packing variants, execute them in an isolated environment and measure which transformations materially change static or heuristic detection.

```text
INSPECT  ─►  PACK  ─►  VALIDATE  ─►  EXECUTE  ─►  ANALYZE  ─►  MUTATE  ─►  COMPARE
```

Planned offensive research work:

- randomized section names, metadata and layout;
- interchangeable compression and encoding strategies;
- multiple runtime restoration paths;
- deterministic builds for debugging;
- randomized builds for mutation experiments;
- structural, entropy and hash comparison;
- batch generation of packing variants;
- isolated execution validation;
- antivirus and EDR result history;
- measurement of stable packer-specific fingerprints.

The target is not a magic “undetected” button. The target is a loader whose observable artifacts can be changed, tested and compared instead of blindly trusted.

## Hard limits

OBSIDIAN rejects states it cannot restore safely.

- PE32 and non-AMD64 binaries are rejected.
- Bound imports are rejected.
- A PE without relocations requires its preferred image base.
- Static TLS is prepared only for the initial thread.
- TLS callbacks are not dispatched automatically for later threads.
- Original PE resources are not registered with the host image.
- Authenticode signatures are invalidated by packing.
- Encryption and polymorphism are not implemented.
- `.obsram`, `OBSD` and the current stub are stable detection surfaces.
- Only repository fixtures have been validated end to end.

These are engineering limits, not fine print. If OBSIDIAN cannot prove that the restored image is safe to launch, it should fail hard.

## Roadmap

- [x] Strict PE32+ validation
- [x] Compression and embedded payload format
- [x] Runtime section injection
- [x] Memory-layout reconstruction
- [x] Relocations, imports and unwind registration
- [x] Main-thread TLS and callbacks
- [x] Final memory protections
- [x] Entry-point transfer
- [x] Argument and exit-code fixtures
- [ ] Resource support
- [ ] TLS support for later threads
- [ ] Deterministic automated fixtures
- [ ] Mutation engine
- [ ] Structural analysis reports
- [ ] Isolated execution harness
- [ ] Detection benchmarking and history
- [ ] Stable command-line interface

## Responsible use

The code is offensive by design. Do not water that down, and do not confuse it with permission.

Legitimate uses include packer research, reverse engineering, detection engineering, EDR validation, malware-analysis training and authorized red-team laboratories. Deploying packed code against systems, users or organizations without explicit authorization is outside the scope of this project.

You are responsible for the payloads you pack, the machines you execute them on and the laws that apply to your work.

## License

No license has been selected. Until a license file is added, this repository does not grant permission to copy, modify or redistribute the code.

---

<p align="center">
  <img src="assets/branding/obsidian-emblem.png" alt="OBSIDIAN emblem" width="96">
</p>

<p align="center">
  <strong>BLACK GLASS. VIOLET FRACTURE. NO FALSE PROMISES.</strong>
</p>
