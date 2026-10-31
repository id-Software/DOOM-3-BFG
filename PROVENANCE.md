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
| `src/rgd3_common.c`, `.h` | `neo/framework/common_frame.cpp` (Frame, idGameThread::Run / RunGameAndDraw, ProcessGameReturn), `Common_load.cpp` (StartNewGame, MoveToNewMap, UnloadMap, ExecuteMapChange, SaveGame, LoadGame, map/devmap/saveGame/loadGame), `Common.cpp` (com_engineHz numerator/denominator, Stop, LeaveGame, ProcessEvent, CreateMainMenu, disconnect), `Common.h` (FRAME_TO_MSEC, MSEC_TO_FRAME_*), `Common_menu.cpp` (StartMenu, MenuEvent) | frame order, pause rule, game tick accumulator and its cvars, latched game return, map change order, event routing. No wipes, loading GUI, demos, lobby waits or net snapshots yet |
| `src/rgd3_usercmd.c`, `.h` | `neo/framework/UsercmdGen.cpp/.h`, `neo/framework/KeyInput.cpp` (GetUsercmdAction) | `usercmd_t` field layout, `BUTTON_*` / `IMPULSE_*` / `usercmdButton_t` / `inhibit_t` values, `userCmdStrings` table, cvar names/defaults/flags/ranges, `buttonState_t` toggles, MouseMove 8-sample smoothing and 1000 delta cap, AdjustAngles, KeyMove, CmdButtons, CircleToSquare, HandleJoystickAxis, JoystickMove / JoystickMove2, JoypadFunction transfer curves, MakeCurrent order and 90° pitch clamp, impulse sequencing, idUserCmdMgr ring semantics (128 × MAX_PLAYERS, overflow-to-middle, read/write frame rules) |
| `src/rgd3_gamepad.c`, `.h` | `neo/sys/win32/win_input.cpp` (idJoystickWin32: joyRemap, PostInputEvent, PushButton, SetRumble), `neo/sys/sys_public.h` (joystickAxis_t, sys_jEvents), `neo/framework/common_frame.cpp` (ProcessGameReturn rumble gating), `UsercmdGen.cpp` (in_useJoystick, in_joystickRumble); RBDOOM-3-BFG `neo/sys/sdl/sdl_events.cpp` for reference only (guide → JOY11) | XInput button → K_JOY numbering, stick/trigger → K_JOY_STICK* / K_JOY_TRIGGER* at range 16384, SE_JOYSTICK value scaling, per-device key dedupe, rumble clamp and master-device gating |
| `src/rgd3_console.c`, `.h` | `neo/framework/Console.cpp`, `ConsoleHistory.cpp/.h`, `neo/idlib/Str.cpp` (g_color_table) | text buffer of colour<<8\|char cells, Linefeed/Print wrap rules, NUM_CON_TIMES notify, con_speed/con_notifyTime/con_noPrint, ProcessEvent/KeyDownEvent key handling, DrawSolidConsole/DrawNotify/DrawInput layout, clear/conDump, 64-line history with up/down points and consoleHistory.txt, colour table |
| `src/rgd3_editfield.c`, `.h` | `neo/framework/EditField.cpp/.h` | cursor/scroll/insert/overstrike rules, control-code CharEvent, word moves, AutoComplete passes (FindMatches common prefix, FindIndexMatch cycling, PrintMatches/PrintCvarMatches) |
| `src/rgd3_cmd.c` (completion) | `neo/framework/CmdSystem.cpp/.h` | CommandCompletion, ArgCompletion, ArgCompletion_FolderExtension with its cache, ArgCompletion_DeclName, inline ArgCompletion_* helpers |
| `src/rgd3_cvar.c` (completion) | `neo/framework/CVarSystem.cpp/.h` | CommandCompletion, ArgCompletion, bool cvars defaulting to ArgCompletion_Boolean |
| `src/rgd3_screenshot.c`, `.h` | `neo/renderer/RenderSystem_init.cpp` (R_ScreenShot_f, R_ScreenshotFilename, TakeScreenshot) | argument forms, shot%05i numbering, console close before the shot, 24-bit TGA header and bottom-up BGR layout |
| `src/rgd3_bitmsg.c`, `.h` | `neo/idlib/BitMsg.h`, `neo/idlib/BitMsg.cpp` | idBitMsg bit layout (LSB-first packing via a 64-bit staging value, signed negative bit counts), overflow policy and messages, 7-bit and '%' string rules, ReadData/ReadString semantics, DirToBits/BitsToDir, quantised float arithmetic (integer scale when max <= storeMax), delta helpers, WriteDeltaDict/ReadDeltaDict wire format |
| `src/rgd3_fs.c` (memory files, typed IO) | `neo/framework/File.cpp` (idFile::Read*/Write*, WriteBig/ReadBig in File.h, idFile_Memory) | little-endian int/short/float, one-byte bool, int-length-prefixed strings without NUL, float vectors/matrices, big-endian WriteBig; memory-file read/write modes, MakeReadOnly/MakeWritable rewind, Clear, PreAllocate, trailing NUL |
| `src/rgd3_dict.c` (`rgd3_dict_write_ini`, `rgd3_dict_read_ini`) | `neo/idlib/Dict.cpp` (WriteToIniFile, ReadFromIniFile), `neo/idlib/Str.cpp` (CStyleQuote, CStyleUnQuote) | INI layout (sorted keys, prefix-less first, `[prefix]` sections split at the last '/', blank-line rule), quoting escapes, lexer flags and `[ ] =` punctuation, duplicate-key warning |
| `src/rgd3_savegame.c`, `.h` | `neo/sys/sys_savegame.h/.cpp`, `neo/sys/sys_session_savegames.cpp`, `neo/sys/win32/win_savegame.cpp`, `neo/framework/Common_load.cpp` (ScrubSaveGameFileName, SaveGame, LoadGame, OnLoadFilesCompleted), `neo/framework/File_SaveGame.h` | saveGameError_t / saveGameType_t values and error names, SAVEGAME_DETAIL_FIELD_* keys, game.details file name and checksum rule, language + checksum added at save, load checks (missing non-optional -> CORRUPTED, details missing/damaged/newer version), temp-file-then-rename writes and error mapping, enumerate-by-details, delete order, scrub character set, MAX_FILES_WITHIN_SAVEGAME / MAX_FOLDER_NAME_LENGTH, 256 KB block size, cvar and command names. Container format, writer thread and newest-wins are new |
| `src/rgd3_session.c`, `.h` | `neo/sys/sys_session.h` (sessionState_t, idMatchParameters, GAME_MODE/MAP_SINGLEPLAYER), `sys_session_local.cpp` (StartMatch, LoadingFinished, QuitMatchToTitle) | local session states without lobbies |
| `src/rgd3_hooks.h` | `neo/d3xp/Game.h` (idGame, gameReturn_t), `neo/renderer/RenderSystem.h`, `RenderWorld.h`, `neo/sound/sound.h` | the subsystem entry points the framework calls |
| `src/rgd3_game_stub.c` | `neo/d3xp/Game_local.cpp` (GetSaveGameDetails), `neo/sys/sys_savegame.h` (SAVEGAME_DETAIL_FIELD_* keys) | detail field names; everything else is stub behaviour |
| `src/rgd3_jobs.c`, `.h` | `neo/idlib/ParallelJobList.h/.cpp` | job list API, jobListId_t / priority / parallelism values, MAX_REGISTERED_JOBS, jobs_* cvars; execution is inline (phase A) |
| `src/rgd3_mem.c` (arenas) | `neo/renderer/tr_local.h` (frameData double buffering) | double-buffered frame memory |
| `src/rgd3_sysevents.c` (run loop) | `neo/framework/EventLoop.cpp` (RunEventLoop, ProcessEvent), `neo/sys/win32/win_input.cpp` (Sys_PollMouseInputEvents), `neo/sys/win32/win_main.cpp` (Sys_GenerateEvents / Sys_ConsoleInput) | command buffer before each event, key state before dispatch, polled mouse deltas, one text-console line per frame |
| `src/rgd3_fs.c` (map containers) | `neo/framework/FileSystem.cpp` (BeginLevelLoad, UnloadMapResources) | per-map `maps/<name>.resources` mounting |
| `src/rgd3_math.c`, `.h` | `neo/idlib/math/Math.h/.cpp` | idMath constants, InvSqrt/Sqrt semantics, 16-bit approximations, Ftoi family, clamps, angle helpers, ANGLE2SHORT and related macros |
| `src/rgd3_math.c`, `.h` | `neo/idlib/math/Vector.h/.cpp`, `Matrix.h/.cpp`, `Quat.h/.cpp`, `Angles.h/.cpp`, `Rotation.h/.cpp`, `Plane.h/.cpp`, `Pluecker.h/.cpp`, `Random.h` | vectors 2-6, Mat2/3/4 (axes as rows, Mat4 row-major) incl. double-precision inverses, quats and compressed quats, angles, rotations, plane convention (Dist = -d) and PLANESIDE/PLANETYPE values, Pluecker coordinates, idRandom/idRandom2 LCGs |
| `src/rgd3_math.c`, `.h` | `neo/idlib/bv/Bounds.h/.cpp`, `Sphere.h/.cpp`, `Box.h/.cpp` | bounding volumes, ToPoints corner order, plane side and intersection tests |
| `src/rgd3_math.c`, `.h` | `neo/idlib/geometry/JointTransform.h/.cpp`, `DrawVert.h/.cpp`, `neo/idlib/math/Simd_Generic.cpp` | 32-byte joint quats, 48-byte joint matrices, joint conversion/blend/transform loops, idDrawVert/idShadowVert layouts, half floats bit-exact, normal byte encoding (SSE2 rounding path) |

## From other projects

| rgd3 file | Source | License |
|---|---|---|
| `src/rgd3_overlay.c` | adapted from `rg_doom/src/rgd_overlay.c` (same author); lower case, punctuation and descender glyphs added to the 5x7 demo font for rgd3 | GPL-3.0-or-later |
| `stb_image_write.h`, `miniz_tinfl.c` (included from `rg_image/third_party`) | PNG screenshots, zlib deflate (`stbi_zlib_compress`) and inflate for saves | public domain / MIT |
| `demo_font.h` (included from `rg_text/examples/assets`) | rg_text | MIT |
| `setup_rg.bat`, `build.bat` structure | `boomer/setup_rg.bat`, `boomer/build.bat` (same author) | — |

## Libraries used (not vendored)

| Library | Version | License |
|---|---|---|
| rg_core, rg_text | sibling checkouts (`..\rg_core`, `..\rg_text`) | MIT |
| SDL | 3.4.10 (`C:\libs\SDL3-3.4.10`) | zlib |
| SDL_shadercross | 3.0.0 (`C:\libs\SDL3_shadercross-3.0.0-windows-VC-x64`) | zlib |
