<p align="center">
  <img src="assets/branding/obsidian-readme-banner.png" alt="OBSIDIAN — x86-64 PE packer for anti-detection research" width="100%">
</p>

<p align="center">
  <strong>Open-source x86-64 PE packer for anti-detection research.</strong>
</p>

OBSIDIAN is intended to transform Windows executables, minimize repeatable static signatures, and measure how packing and mutation affect antivirus detection. It is an offensive-security research project: the objective is not disguised as compression, optimization, or intellectual-property protection.

The long-term interface is deliberately simple:

```console
obsidian input.exe
```

Behind that command, OBSIDIAN is expected to inspect the input PE, build a packed executable, validate the result, run controlled analysis, mutate the packing configuration, and retain comparable measurements across builds.

> [!IMPORTANT]
> **Current status: pre-implementation.** The repository contains a minimal buildable executable scaffold, not a working packer. Everything under **Target state** describes the intended finished project, not features that already exist. Capabilities will only be marked as implemented after they have been built and tested.

## Why this exists

Common public packers are easy to fingerprint. Their section layouts, metadata, stubs, and other structural artifacts can become signatures of their own. Packing a clean executable with a well-known tool can therefore increase its detection rate instead of reducing it.

OBSIDIAN explores the opposite direction: a packer designed around measurable variation rather than a single recognizable output format. The project studies which PE transformations survive execution, which artifacts remain stable enough to identify, and how different builds change static and heuristic detection.

This is anti-detection research. No euphemism is needed. It is also not a promise of “FUD,” universal bypass, or permanent invisibility. Detection is an adversarial and constantly changing measurement, not a feature that can be guaranteed.

## Target state

The finished tool is intended to provide:

- strict parsing and validation of Windows PE32+ x86-64 executables;
- packing of executable content into a compressed and/or encoded representation;
- a runtime restoration stub capable of reconstructing a valid execution state;
- controlled handling of entry points, sections, imports, relocations, memory permissions, and integrity metadata;
- reproducible builds for debugging and randomized builds for mutation experiments;
- configurable transformations that reduce stable packer-specific signatures;
- automated structural comparison between the original and packed binaries;
- execution checks in an isolated test environment;
- batch generation of packing variants;
- machine-readable reports for size, entropy, hashes, PE structure, configuration, and detection results;
- comparison of multiple builds to identify transformations that materially affect detection.

The intended workflow is iterative:

```text
inspect -> pack -> validate -> execute -> analyze -> mutate -> compare
```

## Technical direction

- **Platform:** Windows
- **Format:** PE32+ / x86-64
- **Primary language:** C
- **Low-level code:** x86-64 assembly only where justified
- **Build discipline:** `-Wall -Wextra -Werror`, explicit ownership, strict error handling
- **42 constraints:** the current subject, authorized functions, Norm, and Norminette requirements will be applied once they are present in the repository

The implementation should remain small enough to audit. A working vertical slice takes priority over a large collection of empty abstractions.

## Build

Requirements:

- a C++17 compiler;
- GNU Make.

Build with all detected logical processors:

```console
make parallel
```

GNU Make's native parallel syntax is also supported:

```console
make -j
```

For the current test phase, the build writes the main `obsidian` executable and the separate `obsidian-stub` runtime executable directly to the repository root. Windows builds use the `.exe` extension. Use `make clean` to remove every generated build artifact.

The current packer scaffold accepts one runtime stub path:

```console
obsidian.exe obsidian-stub.exe
```

The current test copies the supplied stub to `obsidian-pack.exe` in the repository root, then appends the five ASCII bytes `hello` as a PE overlay. It does not yet parse or modify PE structures.

## File helpers

The C++ `file` module is used for binary file operations. It currently provides `copy`, `clear`, `append`, `write_at`, and `size`, so later packing code can write to the end of a file or to an exact byte offset without duplicating stream-handling code.

## Research scope

OBSIDIAN is built for controlled offensive-security research, packer analysis, reverse engineering, defensive testing, and authorized laboratory evaluation. It may be used to study how security products react to binary transformations and where static or heuristic detections originate.

It is not authorization to deploy packed payloads against systems, users, or organizations without explicit permission. Users are responsible for complying with applicable law, institutional rules, software licenses, and the scope of every assessment.

## Project principles

- Be explicit about offensive capability and intended use.
- Never claim a bypass that has not been reproduced.
- Never present planned functionality as implemented.
- Keep transformations observable, testable, and reversible during development.
- Record the exact configuration behind every measurement.
- Treat antivirus results as time-bound data, not permanent truth.
- Prefer documented PE behavior over fragile tricks unless the experiment specifically targets edge cases.

## Roadmap

- [ ] PE32+ parser and validator
- [ ] Minimal pack-and-restore proof of concept
- [ ] Runtime stub and entry-point transfer
- [ ] Import and relocation strategy
- [ ] Integrity and failure handling
- [ ] Deterministic test fixtures
- [ ] Mutation engine
- [ ] Structural and entropy analysis
- [ ] Isolated execution harness
- [ ] Detection benchmarking and result history
- [ ] Stable command-line interface

## Visual identity

OBSIDIAN uses a restrained visual system built around black mineral surfaces, cold metal, sharp fractures, and a single controlled violet energy accent. It should feel low-level, hostile, and precise without falling into generic hacker or gaming aesthetics.

| Role | Color | Hex |
| --- | --- | --- |
| Obsidian | Near black | `#050607` |
| Graphite | Structural gray | `#17191D` |
| Mineral | Cold silver | `#C4C9D0` |
| Fracture | Electric violet | `#7C2CFF` |

The violet accent should remain rare. It identifies the fracture and active states; it is not a background color. Typography should be wide, uppercase, geometric, and highly legible. Effects stay controlled: no Matrix code, hooded figures, skulls, circuit-board wallpaper, excessive neon, or fake terminal decoration.

Brand assets:

- [`obsidian-readme-banner.png`](assets/branding/obsidian-readme-banner.png) — primary repository header;
- [`obsidian-emblem.png`](assets/branding/obsidian-emblem.png) — transparent standalone emblem.

## License

No license has been selected yet. Until a license file is added, the repository should not be treated as granting permission to copy, modify, or redistribute the code.
