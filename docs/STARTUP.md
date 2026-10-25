# OBSIDIAN — Phase de démarrage (implémentée)

Ce document décrit la phase de démarrage, maintenant implémentée dans
`run_tls_callbacks()` et `launch_original()` : le stub exécute les
callbacks TLS puis transfère réellement l'exécution à l'entry point du
programme original restauré dans `.obsram`.

## 1. État garanti par le stub avant le transfert

Chaque étape est validée par `validate_restored_image()` :

1. Image reconstruite : headers byte-fidèles + sections copiées en
   disposition RVA relative au début de `.obsram`.
2. Relocations DIR64 appliquées : delta = adresse réelle de `.obsram` −
   `ImageBase` original.
3. IAT intégralement résolue (slot par slot non nul).
4. Table d'exceptions `.pdata` enregistrée : `RtlAddFunctionTable(pdata,
   count, obsram)`.
5. TLS : index dérivé des index réellement attribués par le loader (lus
   dans `AddressOfIndex` de chaque module TLS), slot ajouté dans
   `TEB->ThreadLocalStoragePointer`, bloc du thread principal initialisé
   (raw data + zero fill).
6. Protections finales : OU de permissions par page 4 Ko.
7. Entry point : appartient à une section EXEC et la page réelle est
   exécutable (`VirtualQuery`).

## 2. Ce que le transfert fait, dans l'ordre (implémenté)

1. **Callbacks TLS** : exécutés avec `(base_restaurée, DLL_PROCESS_ATTACH,
   NULL)`, validés et bornés avant le premier appel. Sans eux, tout
   `thread_local` C++ du original resterait non initialisé.
2. **Appel de l'entry point** : `((int (*)(void))(obsram +
   entry_point_rva))()` — un entry d'EXE (`mainCRTStartup`) ne prend pas
   d'argument, il lit argc/argv dans le PEB du processus courant.
3. **Code de sortie** : l'entry retourne `int` dans RAX ; si le original
   appelle `ExitProcess` via son propre CRT, il ne revient jamais. Sinon
   `ExitProcess(valeur_retournée)`.

## 3. Convention d'appel Windows x64 (rappel)

- Arguments 1-4 : RCX, RDX, R8, R9 (l'entry d'EXE n'en prend aucun).
- Retour : RAX.
- Volatiles (clobberables par l'appelé) : RAX, RCX, RDX, R8-R11, XMM0-5.
- Non volatiles à préserver : RBX, RBP, RDI, RSI, R12-R15, XMM6-15.
- Alignement pile : RSP ≡ 8 (mod 16) à l'entrée de la fonction appelée ;
  un `call` normal garantit cet alignement.
- Shadow space : 32 octets réservables par l'appelant.

## 4. Environnement disponible pour le original

- Ligne de commande, chemins, PEB : ceux de `output.exe` — l'argument
  forwarding est vérifié par test (le original voit les vrais argv).
- Deux CRT cohabitent (celui du stub, déjà initialisé, puis celui du
  original via son entry). Chacun gère son propre état.
- `GetModuleHandle(NULL)` renvoie la base de `output.exe`, pas celle du
  original — le `__ImageBase` du original est relogé vers `.obsram`.
- Ressources du original : non enregistrées (directory RESOURCE absent
  des headers de sortie).

## 5. Ce qui est validé et ce qui ne l'est pas

- L'exécution réelle est validée uniquement sur les binaires du projet
  (`obsidian.exe`, `build/obsidian-stub.exe`) : le original reconstruit
  s'exécute, affiche son propre message et retourne son propre code de
  sortie. Aucun binaire tiers n'a été testé.
- Programmes dépendant de TLS callbacks : supportés pour le thread
  principal (callbacks exécutés) ; les threads créés ensuite par le
  original n'exécutent pas ces callbacks (limitation).
- Imports bound : refusés. EXE sans relocations : refusés si la base
  réelle diffère de l'ImageBase. Ressources : non supportées.
