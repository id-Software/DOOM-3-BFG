# Repository guidelines

## Structure

- `src/` holds all engine code. `src/all.c` is the single translation unit:
  it includes the rg_* headers once, then `src/rgd3.h`, then every `.c` file.
  Add new `.c` files to `all.c` in dependency order and new headers to `rgd3.h`.
- `shaders/src/` holds HLSL sources; `shaders/Compiled/{DXIL,SPIRV,MSL}/` holds
  `build_shaders.bat` output, committed.
- `neo/`, `doomclassic/` and `base/` are the untouched id Software release,
  kept as reference. Never build or edit them.
- `tools/` for standalone utilities, `docs/` for format and design notes.
- `PLAN.md` holds decisions and milestones, `PROVENANCE.md` every lifted
  algorithm, format or table.

## Build and run

- `build.bat [debug|dev|release|profile|clean]`: one `cl` call over
  `src\all.c`. Outputs `rgd3_<mode>.exe` (release: `rgd3.exe`) in the root.
- `build_shaders.bat`: shadercross over `shaders\src` and the rg_text shaders.
- rg libraries resolve through `setup_rg.bat`: `RG_CORE_DIR`, `RG_TEXT_DIR`,
  `RG_GUI_DIR`, `RG_IMAGE_DIR`, `RG_AUDIO_DIR` default to `..\rg_<name>`.
  SDL3 comes from `SDL3_DIR` (default `C:\libs\SDL3-3.4.10`). Never hard-code
  other paths.
- Game data is auto-detected from Steam; override with
  `+set fs_basepath "<install dir>"` or `RGD3_BASEPATH`.
- Commands can be piped to stdin, one per line, e.g.
  `"listResources _common`nquit" | .\rgd3_dev.exe`.

## Style

- C11, MSVC `/W3` clean. Tabs, Allman braces, `char* p`.
- Functions `rgd3_<module>_<verb>`, types `Rgd3Thing`, constants and macros
  `RGD3_*`, files `src/rgd3_<module>.c/.h`.
- `typedef struct X { ... } X;`, `= {0}` descriptors, `int` 1/0 for success.
- Each header starts with the `RGD3_ALL_C` guard and never includes rg_* or
  SDL headers itself.
- Cvars: `static Rgd3Cvar cv_<name> = RGD3_CVAR(...)`, registered from the
  module's `rgd3_<module>_register()`, which `src/rgd3_registry.c` calls.
  Keep the original cvar names and flag semantics.

## Memory

- `rgd3_perm_alloc` for data that lives until exit.
- `rgd3_heap_alloc(size, RGD3_TAG_*)` for anything freed or replaced; the
  tag counters (`memTags`) are the leak check.
- rg_core containers need an arena and never free; use them only with
  arenas that are reset as a whole.

## Text, dicts and decls

- `Rgd3Token`s own their text: zero-initialise, copy with
  `rgd3_token_copy`, never struct-assign, `rgd3_token_free` when done.
  Lexers and parsers likewise: `*_init` then `*_free`.
- `Rgd3Dict` is zero-initialisable; assign with `rgd3_dict_assign`, free with
  `rgd3_dict_clear`. Keys are case-insensitive and pooled.
- A decl type is a `Rgd3DeclTypeDesc` (keyword, body size, parse/free/
  default/print callbacks). Its body struct starts with `Rgd3DeclBody`;
  finds return the body. Parsers only warn, never drop, and follow the
  original type's error path: most call `rgd3_decl_make_default` and return
  0, but the PDA family (pda/email/video/audio) returns 0 and stays parsed
  with partial data, as `DeclPDA.cpp` does. The manager ignores the result.
- Console arguments come through the real lexer: `*`, `#`, `-` and other
  punctuation split arguments. Filters use `rgd3_cmdargs_match`
  (arguments joined without spaces), as the original list commands do.

## Errors and threads

- `rgd3_error_drop` only on the main thread; every `*_shutdown` /
  map-teardown path must be idempotent because a drop can arrive mid-way.
- A drop must not leak. Objects that own memory register with
  `rgd3_error_unwind_register` while alive (lexers, parsers and long tokens
  already do; `rgd3_mapfile_parse` registers its map); only main-thread
  stack objects are recorded. A drop frees every recorded object registered
  after its guard was pushed, wherever it is declared, including inside the
  guarded block. So a stack object that outlives the guarded block but was
  registered inside it comes back empty after a drop: free functions must
  leave objects empty, valid and safe to free again. Heap-owned partial
  results go into their owner (e.g. `Rgd3MapFile.pending_*`) so the
  owner's free releases them. Never let a registered stack object go out of
  scope without its free call. The self-test's drop tests compare tag
  counters and the registry size around deliberate drops.
- Locals modified between `setjmp` and `longjmp` must be `volatile`.
- Worker threads never call into fs, decls or cvars.

## Provenance

Every algorithm, binary layout or table taken from `neo/` gets a
`PROVENANCE.md` row and a file-header comment naming the original file.
Write new C code; do not paste the C++.

## Testing

- M0: build all four modes; run the piped command checks in PLAN.md
  Acceptance.
- From M1: `rgd3_dev.exe --self-test` (exit code 0 = pass). It runs without a
  window, checks the foundation modules and the shipped data, and reports
  heap tags still allocated after shutdown as leaks. New modules add their
  checks to `src/rgd3_selftest.c`.
- Write and edit C sources with the Write/Edit tools. The Bash tool collapses
  `\\` to `\`, which corrupts escapes in heredocs and inline scripts.
