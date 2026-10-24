# LZMA SDK (vendored)

These sources come from the LZMA SDK shipped with 7-Zip, taken from the
`C/` directory of https://github.com/ip7z/7zip.

The LZMA SDK is placed in the **public domain** by Igor Pavlov. See the
header comments in each file for the original notice.

## Files

| File | Role |
| --- | --- |
| `LzmaLib.c/.h` | `LzmaCompress` / `LzmaUncompress` entry points |
| `LzmaEnc.c/.h` | Encoder |
| `LzmaDec.c/.h` | Decoder |
| `LzFind.c/.h`, `LzHash.h` | Match finder |
| `Alloc.c/.h` | Allocation hooks |
| `CpuArch.c/.h` | Architecture feature detection |
| `7zTypes.h`, `Compiler.h`, `Precomp.h`, `7zWindows.h`, `Threads.h` | Support headers |

## Build flags

The Makefile compiles these files with `-D_7ZIP_ST -DZ7_ST`, which selects
the single-threaded build. That keeps the runtime stub free of any threading
dependency.

## Updating

Replace the files wholesale and rebuild. Only `LzmaLib.h` and `LzmaDec.h`
are included by project code (`src/compression/lzma.cpp`), so a rename of
those would require a matching change there.
