# Provenance

rgd3 is GPL-3.0-or-later, a derivative of the Doom 3 BFG Edition GPL source
release (see `COPYING.txt` and the additional terms in `README.txt`). Code in
`src/` is new C. This file records what each module takes from the original
release or from other projects.

## From the Doom 3 BFG Edition source (`neo/`)

| rgd3 file | Original | What was taken |
|---|---|---|
| `src/rgd3_fs.c` | `neo/framework/File_Resource.cpp` (Init), `neo/framework/File_Resource.h`, `neo/framework/File.cpp` (ReadString, ReadBig) | `.resources` container layout: magic, table offset/length, mixed-endian entry records, lowercase names, later-entry-wins duplicates |
| `src/rgd3_fs.c` | `neo/framework/FileSystem.cpp` (AddGameDirectory, FindFile, ListFiles, Dir_f, Path_f) | search-path order, container-before-loose priority (`fs_resourceLoadPriority`), cvar names, `dir`/`dirtree`/`path` output |
| `src/rgd3_cvar.c` | `neo/framework/CVarSystem.cpp/.h` | `cvarFlags_t` values, value normalisation for bool/integer/float/string-list cvars, set/seta/sets/sett/setu/toggle/reset/listCvars/cvar_restart/cvarAdd behaviour |
| `src/rgd3_cmd.c` | `neo/framework/CmdSystem.cpp/.h` | `cmdFlags_t` values, command buffer splitting on `;` outside quotes and newlines, `wait`, exec/vstr/echo/listCmds behaviour |
| `src/rgd3_cmdargs.c` | `neo/idlib/CmdArgs.cpp`, `neo/idlib/Lexer.cpp` | tokenizer rules for command lines (lexer flags used by TokenizeString, negative number join, `$cvar` expansion), multi-character punctuation list |
| `src/rgd3_keys.c`, `src/rgd3_keys.h` | `neo/sys/sys_public.h` (keyNum_t), `neo/framework/KeyInput.cpp` | key number values and key name / alias table (needed so shipped configs resolve), bind/unbind/unbindall/listBinds behaviour |
| `src/rgd3_sysevents.c`, `.h` | `neo/sys/sys_public.h` (sysEvent_t), `neo/framework/EventLoop.cpp`, `neo/sys/win32/win_main.cpp` | event types and fields, queue-overflow policy, event-loop dispatch order |
| `src/rgd3_log.c` | `neo/framework/Common_printf.cpp` | developer-only DPrintf/DWarning, warning list, printWarnings |
| `src/rgd3_error.c` | `neo/framework/Common_printf.cpp` (Error, FatalError) | drop vs fatal policy, three drops within 100 ms escalate to fatal |
| `src/rgd3_str.c` | `neo/idlib/Str.cpp/.h` | case-insensitive compares, path/extension helpers, colour escapes, float formatting, `va()` ring |
| `src/rgd3_time.c` | `neo/sys/win32/win_shared.cpp` | integer millisecond clock semantics |
| `src/rgd3_app.c` | `neo/framework/Common.cpp` (Init, ParseCommandLine, SafeMode, StartupVariable, AddStartupCommands, WriteConfiguration) | startup order, `+` command-line handling, `safe`, config write policy |
| `src/rgd3_platform.c` | `neo/sys/win32/win_main.cpp` | base-path / save-path roles (implementation is new: SDL paths and Steam detection) |
| `src/rgd3_mem.h` | `neo/idlib/sys/sys_alloc_tags.h` | memory tag grouping |
| `src/rgd3_lexer.c`, `.h` | `neo/idlib/Lexer.cpp/.h`, `neo/idlib/Token.cpp/.h` | full idLexer/idToken port: LEXFL_*, TT_*, P_* values, default punctuation table and longest-first chains, white space and comment rules (signed-char bytes >= 0x80 skipped), escape characters (decimal after `\`), string concatenation rules, number classification and NumberValue, Expect/Check/Peek/Skip/Parse* API, ParseBracedSection(Exact), error vs warning policy |
| `src/rgd3_parser.c`, `.h` | `neo/idlib/Parser.cpp/.h` | idParser pre-compiler: `define_t`/`indent_t` layout, `PC_NameHash` (sum of `name[i]*(119+i)`, folded `^>>10 ^>>20`, 2048 buckets), DEFINE_FIXED / BUILTIN_* / INDENT_* values, MAX_DEFINEPARMS 128, TOKEN_FL_RECURSIVE_DEFINE, ReadSourceToken / ExpandDefine / MergeTokens / StringizeTokens rules, directive set, EvaluateTokens operator priorities and 64-value / 64-operator limits, `$evalint`/`$evalfloat` and `#eval`/`#evalfloat` formats, `__DATE__`/`__TIME__` layout, error and warning texts. Deviations (leak and crash fixes) are listed in the file header |
| `src/rgd3_lexer_test.c` | `neo/idlib/Lexer.cpp`, `neo/idlib/Token.cpp` | expected behaviour only (escape, number, white-space and comment quirks); no code |
| `src/rgd3_dict.c` (`rgd3_dict_parse`) | `neo/idlib/Dict.cpp` (Parse) | `{ "key" "value" }` parsing with duplicate warnings |
| `src/rgd3_cmdargs.c` | `neo/idlib/CmdArgs.cpp` (TokenizeString), `neo/framework/CVarSystem.cpp` / `CmdSystem.cpp` (`Args(n,-1)` with spaces removed for list filters) | lexer flags, `-` number join, `$cvar` expansion, list filter rejoin |
| `src/rgd3_str.c` | `neo/idlib/Str.h` (Hash, IHash), `neo/idlib/Str.cpp` (FloatArrayToString), `neo/idlib/hashing/CRC32.cpp` | id string hashes used for hash chains, `ToString(precision)` formatting, CRC-32 (0xEDB88320) |
| `src/rgd3_hashindex.c`, `.h` | `neo/idlib/containers/HashIndex.h/.cpp` | idHashIndex: lazy allocation, chains, granularity, InsertIndex/RemoveIndex shifting, GetSpread, GenerateKey variants |
| `src/rgd3_dict.c`, `.h` | `neo/idlib/containers/StrPool.h`, `neo/idlib/Dict.cpp/.h` | interned ref-counted key pool (case-insensitive) and value pool, idDict Set/Copy/SetDefaults/TransferKeyValues/Delete (order kept)/MatchPrefix/RandomPrefix/Checksum, typed getters (atof/atoi/sscanf), file handle format, showDictMemory/listDictKeys/listDictValues. The pool removes by swapping with the last entry instead of shifting |
| `src/rgd3_lang.c`, `.h` | `neo/idlib/LangDict.cpp`, `neo/framework/Common.cpp` (InitLanguageDict, FilterLangList) | `.lang` parsing (BOM check, comments, quoted key, value to end of line, escapes), `#str_` redirection depth 2, `sys_lang` / `lang_maskLocalizedStrings` |
| `src/rgd3_decl.c`, `.h` | `neo/framework/DeclManager.cpp/.h`, `neo/d3xp/Game_local.cpp` (decl folder registration) | decl types and states, DECL_LEXER_FLAGS, LoadAndParse scanning and redefinition handling, FindTypeWithoutParsing/FindType/DeclByIndex, MakeNameCanonical, ParseLocal/MakeDefault/Purge, level-load purging, list*/print*/listDecls/reloadDecls/touch commands, folder order. Decl text is not Huffman-compressed; checksums are CRC32 instead of MD5 |
| `src/rgd3_decl_basic.c`, `.h` | `neo/framework/DeclTable.cpp`, `DeclEntityDef.cpp`, `DeclSkin.cpp`, `DeclPDA.cpp` | table/entityDef/mapDef/skin/pda/email/video/audio parsers, default definitions, TableLookup, `inherit`, implicit skins, `#str_<decl>_<kind>_<field>` lookups, `g_useOldPDAStrings` |
| `src/rgd3_bin.c`, `.h` | `neo/framework/File.cpp` (ReadBig/ReadInt/ReadString), `neo/idlib/Swap.h` | big-endian values with little-endian string lengths |
| `src/rgd3_manifest.c`, `.h` | `neo/framework/File_Manifest.cpp/.h`, `neo/framework/Common_load.cpp` (manifest name) | `.preload` and file manifest layouts, preload types, `game/` to `maps/` naming |
| `src/rgd3_mapfile.c`, `.h` | `neo/idlib/MapFile.cpp/.h` | `.map` v1/v2 parsing (brushDef/brushDef3/Q3 brushes, patchDef2/3), worldspawn removeEntities/overrideMaterial/forceEntityNames/moveFuncGroups, geometry CRC, ComputeAxisBase/GetTextureVectors |
| `src/rgd3_fs.c` (map containers) | `neo/framework/FileSystem.cpp` (BeginLevelLoad, UnloadMapResources) | per-map `maps/<name>.resources` mounting |
| `src/rgd3_math.c`, `.h` | `neo/idlib/math/Math.h/.cpp` | idMath constants, InvSqrt/Sqrt semantics, 16-bit approximations, Ftoi family, clamps, angle helpers, ANGLE2SHORT and related macros |
| `src/rgd3_math.c`, `.h` | `neo/idlib/math/Vector.h/.cpp`, `Matrix.h/.cpp`, `Quat.h/.cpp`, `Angles.h/.cpp`, `Rotation.h/.cpp`, `Plane.h/.cpp`, `Pluecker.h/.cpp`, `Random.h` | vectors 2-6, Mat2/3/4 (axes as rows, Mat4 row-major) incl. double-precision inverses, quats and compressed quats, angles, rotations, plane convention (Dist = -d) and PLANESIDE/PLANETYPE values, Pluecker coordinates, idRandom/idRandom2 LCGs |
| `src/rgd3_math.c`, `.h` | `neo/idlib/bv/Bounds.h/.cpp`, `Sphere.h/.cpp`, `Box.h/.cpp` | bounding volumes, ToPoints corner order, plane side and intersection tests |
| `src/rgd3_math.c`, `.h` | `neo/idlib/geometry/JointTransform.h/.cpp`, `DrawVert.h/.cpp`, `neo/idlib/math/Simd_Generic.cpp` | 32-byte joint quats, 48-byte joint matrices, joint conversion/blend/transform loops, idDrawVert/idShadowVert layouts, half floats bit-exact, normal byte encoding (SSE2 rounding path) |

## From other projects

| rgd3 file | Source | License |
|---|---|---|
| `src/rgd3_overlay.c` | adapted from `rg_doom/src/rgd_overlay.c` (same author) | GPL-3.0-or-later |
| `demo_font.h` (included from `rg_text/examples/assets`) | rg_text | MIT |
| `setup_rg.bat`, `build.bat` structure | `boomer/setup_rg.bat`, `boomer/build.bat` (same author) | — |

## Libraries used (not vendored)

| Library | Version | License |
|---|---|---|
| rg_core, rg_text | sibling checkouts (`..\rg_core`, `..\rg_text`) | MIT |
| SDL | 3.4.10 (`C:\libs\SDL3-3.4.10`) | zlib |
| SDL_shadercross | 3.0.0 (`C:\libs\SDL3_shadercross-3.0.0-windows-VC-x64`) | zlib |
