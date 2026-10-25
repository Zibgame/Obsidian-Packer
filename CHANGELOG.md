# OBSIDIAN release notes

## v0.1.0-alpha · Shifting Glass

This release replaces the fixed first prototype with a configurable packing pipeline and removes several stable static fingerprints.

### Added

- LZMA compression and ChaCha20 encryption with per-build keys and nonces.
- Compression, encryption and restore-mode registries exposed through the CLI.
- Experimental module-stomp restoration with a safe fallback to the embedded restore region.
- Configurable `--section` names and randomized section names by default.
- `-o` / `--output` for selecting the packed file path.
- Native Linux packer builds and optional mingw-w64 cross-building of the Windows stub.
- Transfer of payload icons, manifests and version resources into the packed executable.
- A Windows application icon for the packer.

### Hardened

- Replaced the plaintext `OBSD` trailer with a fully encoded 48-byte footer.
- Removed the plaintext `OBSIDIAN_KEYSLOT` marker; the masked key now lives in the injected restore section and is located by RVA.
- Removed algorithm names and release diagnostics from the runtime stub.
- Reconstructed the ChaCha20 constants without emitting `expand 32-byte k` as a contiguous string.
- Replaced the fixed `.obsram` name with a randomized or user-selected section name.
- Removed noisy module-stomp candidate names and corrected the restore section's initial permissions.

### Changed

- The packed format is now version 4 and records backend IDs, restore mode, key RVA and restore-region size.
- Build targets now distinguish Windows, Linux and mingw-w64 stub builds.
- The README documents the complete packing and runtime pipeline, current capabilities and hard limits.

### Known limits

- The runtime stub and packed payload target Windows x86-64 only.
- Module stomping remains experimental.
- Static TLS is prepared only for the initial thread.
- The format provides payload confidentiality and structural obfuscation, not authenticity or tamper detection.
- End-to-end compatibility is validated against the repository fixtures, not arbitrary third-party software.

## v0.0.1-alpha · First Fracture

Initial public research build with PE32+ validation, compressed payload embedding, in-memory image reconstruction, relocations, import resolution, unwind registration, main-thread TLS, memory protection restoration and entry-point transfer.
