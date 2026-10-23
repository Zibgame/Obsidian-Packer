<p align="center">
  <img src="assets/branding/obsidian-readme-banner.png" alt="OBSIDIAN — x86-64 PE packer" width="100%">
</p>

<p align="center">
  <strong>A Windows x86-64 PE packer built for offensive research, loader development and detection testing.</strong>
</p>

<p align="center">
  Compress. Restore. Relocate. Resolve. Execute.
</p>

---

# OBSIDIAN

OBSIDIAN is an open-source PE32+ packer for Windows.

It takes a 64-bit executable, compresses it, embeds it inside a runtime stub and rebuilds the original image in memory when the packed file starts. The stub applies relocations, resolves imports, restores TLS state, fixes memory protections and transfers control to the original entry point.

This is not a file compressor wearing a security-themed logo. It is an offensive tooling project built to study executable packing, custom loaders, static signatures and antivirus behavior.

```text
INPUT.EXE  ──►  PACK  ──►  OUTPUT.EXE  ──►  RESTORE IN MEMORY  ──►  EXECUTE
```

> [!WARNING]
> OBSIDIAN manipulates executable files and implements behavior commonly found in malware loaders. Use it only on binaries and systems you own or are explicitly allowed to test. The project is intended for controlled research, reverse engineering and defensive validation.

## Current state

OBSIDIAN is a working execution prototype. It can pack the project test binaries and run the restored program from the `.obsram` section of the generated executable.

Implemented today:

- strict PE32+ / AMD64 input checks;
- XPRESS_HUFF payload compression through the Windows Compression API;
- automatic creation of an uninitialized `.obsram` section in the stub;
- in-memory reconstruction of PE headers and sections;
- `IMAGE_REL_BASED_DIR64` relocation handling;
- import resolution by name and ordinal;
- x64 exception table registration with `RtlAddFunctionTable`;
- main-thread static TLS allocation;
- TLS callback execution with `DLL_PROCESS_ATTACH`;
- page protections rebuilt from the original section flags;
- entry-point validation and execution;
- original exit-code propagation;
- command-line argument forwarding through the current process environment.

This is still research code, not a universal packer. It has been validated against the binaries included in this repository. Compatibility with arbitrary third-party executables is not claimed.

## How it works

### Packing stage

The `obsidian` executable:

1. reads and validates the input as PE32+ x86-64;
2. compresses the complete input file with XPRESS_HUFF;
3. reads the runtime stub;
4. adds a `.obsram` section large enough for the original image;
5. writes the modified stub to `output.exe`;
6. appends the compressed payload and a 20-byte footer.

The packed file ends with:

```text
┌───────────────────────────────┐
│ Modified runtime stub         │
├───────────────────────────────┤
│ Compressed original PE        │
├────────┬───────────┬──────────┤
│ "OBSD" │ packed u64│ raw u64  │
└────────┴───────────┴──────────┘
```

Both sizes are stored as unsigned 64-bit little-endian values.

### Runtime stage

When `output.exe` starts, the stub:

1. opens its own file and reads the footer;
2. extracts and decompresses the embedded PE;
3. locates `.obsram` inside its loaded image;
4. copies the original headers and sections into memory layout;
5. applies base relocations for the real `.obsram` address;
6. loads imported DLLs and rebuilds the IAT;
7. registers the original x64 unwind table;
8. prepares static TLS for the main thread;
9. applies final page permissions;
10. validates the restored image;
11. executes TLS callbacks;
12. calls the original entry point.

No temporary copy of the original executable is written to disk during this process.

## Build

### Requirements

- Windows x86-64;
- a C++17 compiler, tested with MinGW-w64 `g++`;
- GNU Make;
- the Windows Compression API;
- the Cabinet import library (`-lcabinet`).

Build both the packer and its runtime stub:

```console
mingw32-make
```

Or use every detected logical processor:

```console
mingw32-make parallel
```

Generated files:

```text
obsidian.exe
obsidian-stub.exe
```

Remove generated artifacts with:

```console
mingw32-make clean
```

## Usage

With `obsidian-stub.exe` next to the packer:

```console
obsidian.exe payload.exe
```

With an explicit stub path:

```console
obsidian.exe path\to\obsidian-stub.exe payload.exe
```

The packed executable is written to `output.exe`. Run it normally:

```console
.\output.exe
```

Arguments passed to `output.exe` remain visible to the restored program:

```console
.\output.exe first second third
```

## Test payloads

Three small payloads are included:

| Payload | Purpose |
| --- | --- |
| `hello.exe` | Checks execution and exit-code propagation |
| `args_echo.exe` | Checks command-line arguments |
| `tls_demo.exe` | Checks static TLS and TLS callbacks |

Build them with:

```console
mingw32-make examples
```

Example:

```console
.\obsidian.exe .\examples\hello.exe
.\output.exe
echo %ERRORLEVEL%
```

The expected exit code is `42`.

## Project layout

```text
Obsidian-Packer/
├── assets/branding/       Visual identity
├── docs/                  Runtime notes
├── examples/              Small PE test payloads
├── include/
│   ├── compression/       Compression interface
│   ├── file/              Binary file helpers
│   └── pe/                PE structures and parser interface
├── src/
│   ├── compression/       XPRESS_HUFF implementation
│   ├── file/              Binary I/O implementation
│   ├── pe/                PE parsing and section injection
│   ├── stub/              In-memory runtime loader
│   └── main.cpp           Packer entry point
└── Makefile
```

## Known limits

OBSIDIAN deliberately rejects states it does not know how to restore safely.

- PE32 and non-AMD64 binaries are not supported.
- Bound imports are rejected.
- A binary without relocations can only run if `.obsram` matches its preferred image base.
- Static TLS is prepared for the initial thread only.
- TLS callbacks are not automatically dispatched for threads created later.
- PE resources are not registered as resources of the host image.
- Authenticode signatures are not preserved.
- The output has no encryption, polymorphism or mutation engine yet.
- The current footer and section name are stable signatures.
- Only the project fixtures have been tested end to end.

These limits are not hidden behind marketing language. If a feature is not implemented, OBSIDIAN should fail instead of pretending the output is valid.

## Offensive research goals

The long-term objective is to measure how loader design and binary mutation change the observable surface of a packed executable.

Planned work includes:

- configurable payload encoding and compression;
- randomized section names and metadata;
- multiple runtime restoration strategies;
- deterministic builds for debugging;
- randomized builds for signature experiments;
- structural diffing between original and packed files;
- entropy, hash and section-layout reports;
- isolated execution checks;
- batch generation of packing variants;
- antivirus detection history and comparison;
- reduction of stable packer-specific fingerprints.

The intended research loop is:

```text
inspect → pack → validate → execute → analyze → mutate → compare
```

Packing does not make a payload invisible. A known stub, a fixed footer, a repeated section layout or suspicious runtime behavior can be enough to identify it. OBSIDIAN exists to make those artifacts visible, measurable and replaceable.

## Roadmap

- [x] PE32+ validation
- [x] Payload compression
- [x] Runtime section injection
- [x] In-memory image reconstruction
- [x] DIR64 relocations
- [x] Import resolution
- [x] x64 exception registration
- [x] Main-thread TLS restoration
- [x] TLS callback execution
- [x] Original entry-point transfer
- [x] Argument and exit-code tests
- [ ] Resource support
- [ ] Full multi-thread TLS behavior
- [ ] Deterministic test suite
- [ ] Mutation engine
- [ ] Structural analysis reports
- [ ] Isolated execution harness
- [ ] Detection benchmarking
- [ ] Stable command-line interface

## Research ethics

The code is offensive by nature. That does not remove the need for authorization.

Good uses include:

- studying PE loading internals;
- testing EDR and antivirus visibility in a lab;
- building unpacking and detection rules;
- comparing static and behavioral indicators;
- reverse-engineering packer techniques;
- validating defensive controls on owned systems.

Do not deploy packed code against people, infrastructure or organizations without explicit permission. You are responsible for the binaries you pack, the systems you execute them on and the laws that apply to your work.

## License

No license has been selected yet. Until a license file is added, this repository does not grant permission to copy, modify or redistribute the code.
