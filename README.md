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
  <code>COMPRESS</code> · <code>ENCRYPT</code> · <code>RESTORE</code> · <code>RELOCATE</code> · <code>EXECUTE</code>
</p>

---

OBSIDIAN is an open-source **PE32+ packer and in-memory loader**. It takes a Windows x86-64 executable, compresses and encrypts it inside a custom runtime stub, rebuilds the original image in memory and transfers execution to its real entry point.

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
                                   │ add restore section
                                   ▼
┌──────────────┐   validate   ┌──────────────┐   append payload   ┌──────────────┐
│  payload.exe │ ───────────► │   obsidian   │ ────────────────► │  output.exe  │
└──────────────┘ compress +   └──────────────┘                    └──────┬───────┘
                   encrypt
                                                                        │ run
                                                                        ▼
                                                              restore PE in memory
                                                              resolve + relocate
                                                              execute entry point
```

| Packing | Runtime |
| --- | --- |
| Validates PE32+ / AMD64 input | Reads its own embedded payload |
| Compresses with LZMA and encrypts with ChaCha20 | Decrypts and decompresses the original PE |
| Copies the payload's PE resources into the stub | Restores into a randomly named or configurable section whose RVA comes from the footer, or into a stomp region |
| Stores a masked per-build key in the restore section and records its RVA in a fully encoded 48-byte footer with no plaintext magic | Applies `DIR64` relocations and resolves imports |
| Produces a standalone `output.exe` | Restores TLS, permissions and execution |

The original executable is restored directly in memory. No temporary copy of the unpacked PE is written to disk.

## Quick start

### Requirements

- A C++17 compiler and GNU Make
- **Windows target**: MinGW-w64 with C++17 support, plus the MinGW import library for Windows CNG (`-lbcrypt`)
- **Linux target**: no extra library; randomness comes from `/dev/urandom`
- The Windows PE stub is produced by the `windows` target, or cross-compiled on Linux with mingw-w64 (`make stub-mingw`)

### Build

The target platform is chosen explicitly. `windows` builds the packer, the PE runtime stub and the Windows resource; `linux` builds only the packer (the stub stays a Windows binary). With no target, the host is detected automatically.

```console
make windows     # packer + stub + resource (Windows PE)
make linux       # packer only (runs on Linux, still emits a Windows PE)
make parallel    # parallel build for the current host
make examples    # Windows PE fixtures (windows target only)
```

On Windows this creates:

```text
obsidian.exe
build/obsidian-stub.exe
examples/hello.exe
examples/args_echo.exe
examples/tls_demo.exe
```

On Linux it creates a single `obsidian`, with no stub. To obtain a Windows stub from a Linux machine, cross-compile it with mingw-w64:

```console
make stub-mingw  # requires x86_64-w64-mingw32-g++ / -gcc
```

### Pack

```console
.\obsidian.exe .\examples\hello.exe
```

The default stub is detected automatically and the packed binary is written to `output.exe`. Choose the output path with `-o`:

```console
.\obsidian.exe -o packed.exe .\payload.exe
obsidian -o packed.exe ./payload.exe   # same command on Linux
```

Use a custom stub explicitly when needed:

```console
.\obsidian.exe .\path\to\obsidian-stub.exe .\payload.exe
```

Select the packing backends and restore strategy from the command line:

```console
.\obsidian.exe --compression lzma --encryption chacha20 --restore stomp .\payload.exe
```

Set the embedded restore-section name explicitly when needed:

```console
.\obsidian.exe --section .xdata .\payload.exe
```

Section names must be 1 to 8 characters. Without `--section`, the packer generates a random name consisting of a dot followed by seven lowercase letters or digits, such as `.j696vwm`.

The available values are currently `lzma`, `chacha20`, and the `obsram` or `stomp` restore modes. `obsram` is the default. `stomp` is experimental: it attempts to restore the image into a suitable loaded system module and falls back to the restore section whose RVA is stored in the footer when no candidate is available. Run `.\obsidian.exe --help` for the complete syntax.

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
| LZMA compression | **Working** |
| ChaCha20 payload encryption | **Working** |
| Pluggable compression and encryption registries | **Working** |
| Encoded 48-byte footer (no `OBSD` magic) | **Working** |
| Key carried by a section, RVA in footer | **Working** |
| Configurable/random restore-section name | **Working** |
| Diagnostics stripped in release (`OBSIDIAN_DEBUG`) | **Working** |
| Algorithm names kept out of the stub | **Working** |
| Runtime restore-section reconstruction | **Working** |
| Module-stomp restore mode with fallback | **Experimental** |
| In-memory PE reconstruction | **Working** |
| `IMAGE_REL_BASED_DIR64` relocations | **Working** |
| Import resolution | **Working** |
| x64 exception table registration | **Working** |
| Main-thread static TLS | **Working** |
| TLS callbacks | **Working** |
| Section memory protections | **Working** |
| Original entry-point transfer | **Working** |
| Argument and exit-code propagation | **Working** |
| Payload resource transfer | **Working** |
| Full TLS behavior for new threads | **Not supported** |
| Mutation engine | **Planned** |
| Detection benchmarking | **Planned** |

End-to-end execution has been validated on the fixtures shipped with this repository. Compatibility with arbitrary third-party software is not claimed.

## Runtime internals

<details>
<summary><strong>Open the loader pipeline</strong></summary>

When `output.exe` starts, the stub:

1. opens its own executable and reads and decodes the final 48-byte footer;
2. extracts the stored payload and selects its encryption and compression backends;
3. reads the masked 32-byte per-build key from the section at the module base plus `footer.key_rva`;
4. unmasks the key, decrypts with ChaCha20 and decompresses with LZMA;
5. selects the restore section whose RVA comes from the footer, or a compatible module-stomp region;
6. copies PE headers and sections into memory layout;
7. applies base relocations for the selected restore address;
8. loads imported DLLs and patches the original IAT;
9. registers `.pdata` through `RtlAddFunctionTable`;
10. allocates static TLS for the main thread;
11. rebuilds page protections from section characteristics;
12. validates the restored image and entry point;
13. calls TLS callbacks with `DLL_PROCESS_ATTACH`;
14. transfers execution to the original entry point.

The packed file layout is deliberately small:

```text
┌──────────────────────────────────────────────────┐
│ Modified PE runtime stub                         │
│ └── restore section: masked key + image region   │
├──────────────────────────────────────────────────┤
│ LZMA-compressed, ChaCha20-encrypted PE            │
├──────────────────────────────────────────────────┤
│ Encoded footer (48 bytes, masked at rest)        │
└──────────────────────────────────────────────────┘
```

The footer is masked at rest by the xorshift64 keystream using the total packed file size as its seed. Its decoded layout is:

| Offset | Size | Field |
| :---: | :---: | --- |
| 0 | 4 | Internal tag (`OBSD_TAG`, `0x6A09E667`) |
| 4 | 1 | Format version (`4`) |
| 5 | 1 | Compression identifier |
| 6 | 1 | Encryption identifier |
| 7 | 1 | Nonce size |
| 8 | 12 | Nonce storage (`OBSD_NONCE_MAX`) |
| 20 | 8 | Stored payload size |
| 28 | 8 | Original payload size |
| 36 | 1 | Restore mode |
| 37 | 3 | Padding |
| 40 | 4 | Key and restore-region RVA |
| 44 | 4 | Restore-region size |

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
│   ├── cli/               Command-line options
│   ├── crypto/            Encryption interface
│   ├── file/              Binary file helpers
│   ├── loader/            Restore-mode registry
│   ├── packer/            Packer helpers
│   └── pe/                PE parser interface
├── src/
│   ├── algorithm_names.cpp Packer-only algorithm names and CLI lookup
│   ├── cli/               Command-line parser
│   ├── compression/       LZMA backend and registry
│   ├── crypto/            ChaCha20 backend and registry
│   ├── file/              Binary I/O
│   ├── loader/            Restore-mode registry
│   ├── packer/            Random generation and resource transfer
│   ├── pe/                PE parsing and section injection
│   ├── stub/              In-memory runtime loader
│   ├── third_party/lzma/   Vendored public-domain LZMA SDK
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

- randomized metadata and broader layout mutation;
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
- Resource transfer is best effort; malformed or unsupported resource trees leave the stub resources unchanged.
- The `stomp` restore mode is experimental and is not compatible with every payload or host module.
- Authenticode signatures are invalidated by packing.
- Encryption is implemented, but it does not provide authenticity or tamper detection.
- The current stub remains the primary stable detection surface until a mutation engine is implemented; the restore-section name, footer bytes and key location are no longer fixed plaintext anchors.
- Only repository fixtures have been validated end to end.

These are engineering limits, not fine print. If OBSIDIAN cannot prove that the restored image is safe to launch, it should fail hard.

## Roadmap

- [x] Strict PE32+ validation
- [x] Compression and embedded payload format
- [x] ChaCha20 payload encryption and per-build key generation
- [x] Configurable CLI and backend registries
- [x] Runtime section injection
- [x] Encoded 48-byte footer without plaintext magic
- [x] Masked key carried by the restore section, with its RVA in the footer
- [x] Configurable and randomized restore-section names
- [x] Experimental module-stomp restore mode
- [x] Payload resource transfer
- [x] Memory-layout reconstruction
- [x] Relocations, imports and unwind registration
- [x] Main-thread TLS and callbacks
- [x] Final memory protections
- [x] Entry-point transfer
- [x] Argument and exit-code fixtures
- [ ] TLS support for later threads
- [ ] Harden module-stomp compatibility
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

---

# Journal de durcissement anti-fingerprint — 2026-10-25

Cette passe supprime les ancres statiques qui rendaient l'analyse du packer mécanique. Elle couvre les chantiers **A1, A2, A3, B1, B3 et D1**. Les sections plus haut décrivent l'ancien format (footer `OBSD` de 40 octets, `.obsram`, marqueur de keyslot) : elles sont désormais obsolètes sur ces points, remplacées par ce journal.

## Ce qui était exposé

Analyse octet par octet des trois binaires avant la passe :

| Chaîne | Fichier | Offset |
| --- | --- | :---: |
| `OBSIDIAN_KEYSLOT` | `obsidian-stub.exe` / `output.exe` | `0x17800` |
| `expand 32-byte k` | `obsidian-stub.exe` / `output.exe` | `0x18520` |
| `chacha20`, `lzma` | `obsidian-stub.exe` / `output.exe` | registres `.rdata` |
| messages `obsidian: ...` | `obsidian-stub.exe` / `output.exe` | partout |

Chaque étape du déballage avait un point d'ancrage repérable par `strings` : magic `OBSD` en fin de fichier, marqueur du slot de clé, noms d'algorithmes, noms de diagnostic, nom de section `.obsram`.

## A1 — Plus de messages de diagnostic dans le stub

Un macro `OBSIDIAN_LOG(...)` remplace tous les `std::cout` de diagnostic. En release il se réduit à `((void)0)`, donc les littéraux et les appels disparaissent du binaire. En debug (`-DOBSIDIAN_DEBUG`) la sortie revient telle quelle.

```c
#ifdef OBSIDIAN_DEBUG
#define OBSIDIAN_LOG(...) do { std::cout << __VA_ARGS__; } while (0)
#else
#define OBSIDIAN_LOG(...) ((void)0)
#endif
```

`<iostream>` n'est plus inclus dans les unités de compilation partagées avec le stub sauf en debug, ce qui retire au passage l'initialisation des flux côté loader.

Fichiers touchés : `include/obsidian.hpp`, `src/stub/main.cpp`, `src/pe/pe.cpp`, `src/packer/resource.cpp`, `src/main.cpp`. Les sorties non-diagnostiques du packer (résumé de build, `--help`) sont conservées.

Pour déboguer : `mingw32-make CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -DOBSIDIAN_DEBUG"`.

## A2 — Noms d'algorithmes sortis du stub

Le champ `name` est retiré de `crypto::Algorithm` et `compression::Algorithm`. Les tables partagées (`src/crypto/registry.cpp`, `src/compression/registry.cpp`) ne portent plus que `id` et les pointeurs de fonctions, donc plus aucun littéral `"chacha20"` ni `"lzma"` n'est référencé dans le stub.

Les noms, `find()` et `names()` sont déplacés dans `src/algorithm_names.cpp`, compilé uniquement dans le packer (fichier situé à la racine de `src/`, donc hors de `COMMON_SOURCES` du `Makefile`, donc absent du stub). `name_of(id)` remplace l'accès `->name` côté packer.

## A3 — Constantes ChaCha reconstruites proprement

L'ancienne construction d'`"expand 32-byte k"` par un `OR` de caractères était censée éviter le littéral : `g++ -O2` coalesçait quand même les quatre mots en un bloc de 16 octets placé dans `.rdata`, et la chaîne finissait telle quelle dans le binaire.

Les constantes sont maintenant stockées via un store `volatile`, ce qui interdit au compilateur de recoller les quatre mots en une constante vectorielle :

```cpp
void	store_word(uint32_t *slot, uint32_t value)
{
	volatile uint32_t	*target;

	target = slot;
	*target = value;
}

void	state_constants(uint32_t *state)
{
	store_word(&state[0], 0x61707865u);
	store_word(&state[1], 0x3320646eu);
	store_word(&state[2], 0x79622d32u);
	store_word(&state[3], 0x6b206574u);
}
```

Vérifié après build : `expand 32-byte k` n'apparaît plus dans `build/obsidian-stub.exe` ni `output.exe` (recherche octet → `-1`). Fichier : `src/crypto/chacha20.cpp`.

## B1 — Plus de marqueur de keyslot

Le tableau `obsidian_key_slot[]` initialisé avec `OBSIDIAN_KEYSLOT` et l'étape `inject_key()` du packer sont supprimés. Le packer n'écrit plus la clé à une adresse trouvée par marqueur littéral.

À la place, le packer ajoute au stub une section qui porte la clé (32 octets masqués, en tête de section) et note son RVA dans le footer. Le stub lit la clé et calcule l'adresse de restauration depuis sa propre base :

```cpp
base = reinterpret_cast<char *>(GetModuleHandleA(NULL));
key[index] = static_cast<unsigned char>(base[footer.key_rva + index]);
```

Plus aucun marqueur à grepper. `find_obsram()` disparaît aussi : la région de restauration vient de `footer.key_rva` / `footer.region_size`, plus d'un nom de section. Fichiers : `include/obsidian.hpp`, `include/pe/pe.hpp`, `src/pe/pe.cpp`, `src/stub/main.cpp`, `src/main.cpp`.

## B3 — Magic `OBSD` cassée, footer encodé

### Le problème

Le footer était une structure en clair à offset fixe (`EOF-40`) commençant par la signature `"OBSD"`. N'importe qui pouvait le localiser par `strings`, en lire les champs (`stored_size`, `nonce`, ids) et carver le payload directement.

### L'encodage

Le footer fait 48 octets. On construit son clair, puis on le chiffre intégralement avec un **keystream xorshift64 dont la graine dépend de la taille du fichier** — donc unique par sortie, sans constante stockée côté stub :

```c
inline unsigned long long obsidian_footer_avalanche(unsigned long long value)
{
	value = value ^ (value << 13);
	value = value ^ (value >> 7);
	value = value ^ (value << 17);
	return (value);
}

inline void obsidian_footer_cipher(unsigned char *data, unsigned int size, unsigned long long seed)
{
	unsigned long long state;
	unsigned int index;

	state = seed ^ 0x9E3779B97F4A7C15ULL;
	index = 0;
	while (index < size)
	{
		state = obsidian_footer_avalanche(state);
		data[index] = data[index] ^ static_cast<unsigned char>(state & 0xFF);
		index++;
	}
}
```

- **Packer** : `seed` = taille totale (`stub + payload + 48`).
- **Stub** : `seed` = taille de son propre fichier (`GetFileSizeEx`), identique par construction, puis il déchiffre les 48 derniers octets et vérifie un **tag interne** (`OBSD_TAG`, `0x6A09E667`) qui n'existe qu'après décodage.

Résultat : à l'octet, le footer ressemble à du bruit, et chaque `output.exe` produit un footer différent. Plus de `"OBSD"` à offset fixe, plus de structure lisible. Exemple de footer stocké (48 derniers octets) :

```text
e8 0e c4 02 e8 52 c0 d2 dc e3 f4 51 52 3e 67 d5 e3 12 e4 43 1e 66 16
92 6f 61 59 c1 7d c2 5c 66 b0 73 65 4f 95 fa 67 c3 96 8f 94 b5 4a ae 0f d5
```

Limite honnête : c'est de l'obfuscation structurelle solide contre `strings`, YARA et le carving naïf (la graine n'est pas stockée), pas du chiffrement fort — un analyste qui désassemble le stub retrouve le schéma. C'est le même niveau de compromis que le reste des transformations.

### Layout clair du footer (48 octets, avant chiffrement)

| Offset | Taille | Champ |
| :---: | :---: | --- |
| 0 | 4 | tag interne `OBSD_TAG` |
| 4 | 1 | version de format (`4`) |
| 5 | 1 | identifiant de compression |
| 6 | 1 | identifiant de chiffrement |
| 7 | 1 | taille du nonce |
| 8 | 12 | nonce (max `OBSD_NONCE_MAX`) |
| 20 | 8 | taille de la charge stockée |
| 28 | 8 | taille de l'original |
| 36 | 1 | mode de restauration |
| 37 | 3 | padding |
| 40 | 4 | RVA de la clé / région de restauration |
| 44 | 4 | taille de la région de restauration |

Fichiers : `include/obsidian.hpp`, `src/main.cpp`, `src/stub/main.cpp`.

## D1 — Nom de section personnalisable

Le nom `.obsram` est figé dans le code, c'était un mot-logo. Le packer accepte maintenant `--section <name>` (1 à 8 caractères). Par défaut, s'il n'est pas fourni, il **génère un nom aléatoire** à la volée (`.` suivi de 7 caractères `[a-z0-9]`). Le stub ne dépend plus du nom : il utilise le RVA du footer, donc un nom aléatoire par build est gratuit.

```console
.\obsidian.exe --section .xdata .\payload.exe
.\obsidian.exe .\payload.exe            # nom aléatoire, ex: .j696vwm
```

Option ajoutée dans `include/cli/options.hpp` et `src/cli/options.cpp` ; génération dans `src/main.cpp`.

## Correctif — permissions de la section de restauration

La section ajoutée par le packer était mappée `PAGE_EXECUTE_READWRITE` (RWX) au chargement, un indicateur suspect pour les outils d'analyse. Elle est repassée en lecture + écriture (`0xC0000040`), comme l'ancienne `.obsram`. L'exécution n'était pas nécessaire au mapping : `protect_sections` la rétablit au runtime, par section. Fichier : `src/pe/pe.cpp` (`add_payload_section`).

## Vérification

Après rebuild (`mingw32-make parallel`), recherche brute sur `build/obsidian-stub.exe` et sur un `output.exe` fraîchement packé :

| Chaîne | Avant | Après |
| --- | :---: | :---: |
| `OBSIDIAN_KEYSLOT` | présent | absent |
| `expand 32-byte k` | présent | absent |
| `chacha20` | présent | absent |
| `lzma` | présent | absent |
| `obsidian:` (diagnostics) | présent | absent |
| `.obsram` | présent | absent |
| `OBSD` (magic) | présent | absent |

Test de bout en bout : `.\obsidian.exe .\examples\hello.exe` puis `.\output.exe` → exécution du payload, code de sortie `0`. Testé aussi en `--restore stomp --section .xdata`.

## Fichiers modifiés

```text
include/obsidian.hpp                macro OBSIDIAN_LOG, footer v4 (48 o), cipher, tag, offsets
include/crypto/crypto.hpp           champ name retiré, name_of ajouté
include/compression/compression.hpp champ name retiré, name_of ajouté
include/cli/options.hpp             option section
include/pe/pe.hpp                   add_payload_section
src/algorithm_names.cpp             NOUVEAU — noms + find/names/name_of (packer seulement)
src/crypto/registry.cpp             table sans noms, find/names retirés
src/compression/registry.cpp        table sans noms, find/names retirés
src/crypto/chacha20.cpp             constantes via store volatile
src/cli/options.cpp                 option --section
src/pe/pe.cpp                       add_payload_section, diagnostics OBSIDIAN_LOG
src/packer/resource.cpp             diagnostics OBSIDIAN_LOG
src/main.cpp                        footer encodé, section porteuse de clé, nom aléatoire
src/stub/main.cpp                   clé via RVA, footer décodé, find_obsram retiré, logs retirés
README.md                           ce journal
```

Aucun changement de `Makefile` n'était nécessaire pour `algorithm_names.cpp` (pris par `MAIN_SOURCES`).

## Reste à faire (non traité ici)

- **D2** — réduire la grappe d'imports du loader (résolution PEB + hash, au lieu de `VirtualAlloc`/`VirtualProtect`/`LoadLibraryA`/`GetProcAddress`).
- **C** — porter le payload en vraie section PE plutôt qu'en overlay.
- **E** — moteur de mutation au pack-time (junk, permutation de sections, clé d'obfuscation de chaînes par build).
- **F** — l'overlay chiffré reste haute entropie ; à mesurer côté détecteurs.

## Port Linux du packer — 2026-10-25

Le packer tourne maintenant sur Linux et produit le **même PE Windows** que sous Windows. Le stub n'est **pas** porté : c'est un binaire Windows, inchangé.

- **Seul fichier Windows-dépendant côté packer** : `src/packer/random.cpp` (le RNG). `BCryptGenRandom` sous `_WIN32`, `/dev/urandom` ailleurs. Tout le reste (PE, LZMA, ChaCha20, ressources, CLI) est du C++ portable.
- **Makefile** : détection de l'hôte + cibles explicites `make windows` et `make linux`. Windows : packer + stub + ressource (`windres`), lié à `-lbcrypt`. Linux : packer seul, sans `-lbcrypt`, sans stub. La cible `stub-mingw` cross-compile le stub avec mingw-w64 s'il est installé.
- **Nouvelle option `-o, --output <file>`** : chemin de sortie (défaut `output.exe`), pratique pour packer plusieurs binaires d'affilée.
- Sous Linux, le packer lit un stub PE fourni (construit sous Windows ou via `make stub-mingw`) et écrit un PE packé identique octet pour octet au même build sous Windows.

Commandes :

```console
make linux                 # packer seul
make stub-mingw            # stub PE via mingw-w64 (optionnel)
./obsidian -o packed.exe ./payload.exe
```

Fichiers : `src/packer/random.cpp`, `Makefile`, `include/cli/options.hpp`, `src/cli/options.cpp`, `src/main.cpp`, `README.md`.

## Durcissement anti-fingerprint — 2026-10-25

Deux déclencheurs vus par une analyse statique sont supprimés du stub.

**1. Cibles de module stomping neutres.** La liste `g_stomp_candidates` contenait `amsi.dll`, `wintrust.dll`, `msi.dll`, `wldap32.dll`, `winhttp.dll`, `crypt32.dll` et `dbghelp.dll`. Même sans mode stomp, ces noms restent dans le binaire et suffisent à faire dire « charge amsi / winhttp / crypt32 ». Ils sont remplacés par des modules banals et déjà présents partout : `userenv.dll`, `profapi.dll`, `d3d11.dll`, `textinputframework.dll`, `windows.storage.dll`, `msimg32.dll`, `version.dll`. Le stomping fonctionne à l'identique.

**2. Fini les chaînes de debug en release.** `readiness` et `validity` étaient assignées hors `#ifdef OBSIDIAN_DEBUG`, donc les littéraux (`"not ready: environment incomplete"`, `"conditional: TLS callbacks pending"`, `"ready: delay imports self-resolve at first call"`, `"valid"`, `"INVALID"`) restaient dans le stub release. Le calcul et les logs correspondants sont désormais encadrés par `#ifdef OBSIDIAN_DEBUG` ; en release, `(void)tls_ready; (void)delay_dlls;` garde les paramètres utilisés pour `-Werror`.

Fichiers : `src/stub/main.cpp`.

Reste (non traité ici) : `basic_string::substr`, `vector::_M_realloc_append`, `Cannot convert character sequence` et les imports `libstdc++-6.dll` / `libgcc_s_seh-1.dll` proviennent du STL utilisé par le stub.
