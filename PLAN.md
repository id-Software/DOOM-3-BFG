# rgd3 roadmap

rgd3 is a rewrite of the Doom 3 BFG Edition engine in C11 on SDL3 GPU and the
rg_* libraries. It loads the shipped BFG data unchanged from the Steam install
and targets the full single-player game, then co-op through the campaigns.
The original C++ under `neo/` stays in the repository as reference and is
never built.

## Decisions

- **Language and build.** C11 (`/std:c11 /TC`), x64, MSVC `cl` driven by a
  hand-written `build.bat` (debug, dev, release, profile). One translation
  unit, `src/all.c`. No CMake, Ninja, MSBuild or PowerShell build drivers.
- **Libraries.** rg_core, rg_text (and later rg_gui, rg_image, rg_audio) as
  header-only sibling checkouts resolved by `setup_rg.bat` (`RG_*_DIR`
  overrides). SDL 3.4.10 from `C:\libs\SDL3-3.4.10`.
- **Renderer.** SDL3 GPU directly, HLSL compiled offline by SDL_shadercross
  through `build_shaders.bat`. No OpenGL phase.
- **Scope.** Full single player (base game, Resurrection of Evil, The Lost
  Mission), then co-op through those campaigns over SDL3_net (decided
  2026-10-31, see Co-op below). Deathmatch is stripped: no DM/Tourney/Team
  DM/Last Man Standing/CTF rules, CTF entities, MP maps (`d3dm*`, `d3xpdm*`,
  `d3ctf*`) or MP menus. Lobbies, matchmaking, voice chat, leaderboards and
  achievements are dropped. Doom Classic is dropped (rg_doom covers Doom 1/2).
- **Port style.** Rewrite in rg style: `rgd3_<module>_<verb>()`, `Rgd3Type`,
  `RGD3_CONST`, files `src/rgd3_<module>.c/.h`. Algorithms and data formats are
  lifted from the original with a PROVENANCE.md row each; classes are not
  transliterated.
- **Visual target.** The original BFG look with stencil shadow volumes.
  RBDOOM-3-BFG (sibling checkout `C:\Projects\RBDOOM-3-BFG`) is a secondary
  reference only.
- **Data.** Read from the Steam install, never copied into the repository.
  Writes (config, saves, screenshots) go to `SDL_GetPrefPath("superwendel","rgd3")`.
  Original savegame compatibility is not a goal.
- **Registration.** Explicit tables replace the original static constructors:
  each module registers its cvars and commands from `rgd3_<module>_register()`,
  called by `src/rgd3_registry.c`.
- **Errors.** A stack of setjmp guards replaces `idException`. A drop stops the
  map and returns to the innermost guard; three drops in 100 ms or a drop with
  no guard is fatal.
- **Memory.** rg_core pool (2 GB reserved, lazily committed), a permanent
  arena, level and frame arenas (M2), and a tagged SDL heap for anything that
  is freed or replaced.

### Classic restoration track

Keep BFG's engine and data pipeline, restore the 2004 visual behaviour, then
add modern options. Enemy placement changes are out of scope.

1. Flashlight and muzzle-flash shadows on by default (`g_weaponShadows 1`,
   BFG ships `0`), player shadow on (`g_showPlayerShadow 1`). M6.
2. `classic_visuals.cfg`: `r_lightScale 2` (BFG uses 3) and `r_classicShading`
   (2004 specular ramp in an `interaction_classic` shader). M8.
3. BFG optimizations and modern display support kept throughout.
4. Enhanced lighting mode: shadow-map atlas, soft shadows, quality cvars,
   contact shadows, switchable at runtime against Classic. M12.
5. Optional classic gameplay: handheld flashlight in a weapon slot
   (`g_classicFlashlight`, the 2004 `weapon_flashlight` def still ships),
   ammo defs and `pm_*` movement through a `classic/` override directory. M10.

### Co-op

Doom 3 never had co-op, so this is new design on top of the BFG netcode
pieces that fit it. The netcode itself lands in M11, after all three
campaigns work in single player, because co-op exercises everything (AI,
scripts, cinematics, transitions, saves). The game layer is built co-op-ready
from M2 so M11 adds sync instead of rewriting.

**Rules from M2 on (cheap now, expensive to retrofit):**

- Entity numbers 0..7 are player slots (original `MAX_PLAYERS 8`); the
  usercmd ring is per player (128 deep) from M2.
- Game logic never asks for "the local player". It takes an explicit player
  or iterates players. `rgd3_game_local_player()` is for presentation only
  (view, HUD, listener, PDA screen). Each of the 191 original
  `GetLocalPlayer()` sites is classified when ported; logic sites that need a
  co-op decision get a `COOP:` comment.
- The original's `IsMultiplayer()` (150 sites in `neo/d3xp`) mixes two
  questions that co-op splits apart, so it is never ported as one flag:
  - `rgd3_net_role()`: single / host / client. Answers "is this machine
    authoritative" and "do I send or read snapshots". Most of the
    `GetLocalClientNum()` sites (44) and the snapshot/prediction branches
    port to this.
  - `rgd3_game_rules()`: campaign or (never, here) competitive. Answers which
    entityDefs spawn (`player_doommarine_mp`, `_mp` weapons), whether campaign
    weapons, items, cinematics and monsters are suppressed, spawn filtering,
    scoring. Co-op runs campaign rules, so these branches keep their
    single-player behaviour.
  Each `IsMultiplayer()` / `GetLocalClientNum()` site is classified into
  one of the two (or dropped as deathmatch-only) when its class is ported.
- The entity class vtable has `write_snapshot`, `read_snapshot`,
  `client_predict_think` and `client_receive_event` slots from G0 (NULL until
  M11). The original snapshot code in 20 game files (incl. `Physics_AF`,
  `Physics_Monster`, movers, projectiles, items, lights) is ported with its
  class rather than skipped.
- Cross-entity references are `Rgd3EntHandle` (spawn id + slot), which is
  also the network id.
- Script, AI, triggers and physics are server-side state; nothing in them
  reads input or the camera directly.

**M11 design:**

- Transport `rgd3_net`: SDL3_net 3.2.0 UDP datagram socket
  (`C:\libs\SDL3_net-3.2.0`, `SDL3_NET_DIR`, added to build.bat in M11),
  `net_port 27666`, a channel with sequence/ack, fragmentation for large
  snapshots and a reliable ordered stream for commands and events; handshake
  checks protocol version and a decl checksum.
- Messages: `rgd3_bitmsg` from `neo/idlib/BitMsg.cpp`. Snapshots: per-object
  state blobs delta-compressed against the client's acknowledged baseline,
  from `neo/framework/Snapshot*.cpp`, `SnapshotProcessor.cpp`,
  `Compressor.cpp` (LZW) and `neo/d3xp/Game_network.cpp`.
- Listen server: the host runs the whole game. Clients send usercmds,
  predict only their own player (player physics + weapon view model) and
  interpolate everything else. Animated entities replicate animation channel
  state, not joints; ragdolls replicate AF bodies; sounds, fx and decals go
  through reliable events.
- Session: `coop <map>` hosts, `connect <host>[:port]` joins (direct IP; LAN
  discovery only if SDL3_net supports broadcast). Join in progress gets the
  current map and a full snapshot. Only the host saves.
- Co-op rules (cvars `g_coop*`, `si_maxPlayers` default 4, max 8):
  - Extra players spawn around the player start (traced offsets).
  - A dead player respawns at a living player after `g_coopRespawnDelay`; if
    everyone is dead the host reloads the last checkpoint.
  - Keys, PDAs, emails, videos and objectives are shared; health, armor, ammo
    and weapons are per player; weapon pickups stay for the others.
  - `$player1` in scripts is the host's player; script events that target
    "the player" are audited and applied to all players where it matters
    (give, teleport, fade, HUD).
  - Cinematics lock every player to the camera; only the host skips.
  - Any player reaching a level exit moves the whole server
    (`g_coopWaitForAll` optional); per-player persistent info carries over.
  - Monsters target the nearest visible player (original `FindEnemy`
    already iterates clients); `g_coopMonsterScale` optionally scales
    monster health by player count.
  - Expansion mechanics need their own rules. RoE's artifact
    (Helltime slow motion, berserk, invulnerability) changes global game time
    in single player; in co-op it either slows the world for everyone or
    becomes a per-player effect (decide before M11, default: everyone, held
    by one player at a time). The grabber, the Lost Mission maps and their
    level transitions get a pass each.
- Out of scope: NAT traversal, internet server lists, Steam, dedicated
  servers.

## Milestones

| M | Status | Scope | Exit criteria |
|---|---|---|---|
| M0 Skeleton | **done 2026-10-29** | build scripts, `all.c`, mem/str/time/log/error/thread/platform, cmdargs, cvar, cmd, fs with `.resources`, sysevents, keys, stdin console, rg_text overlay, window + SDL GPU clear | see Acceptance M0 |
| M1 Text and formats | **done 2026-10-31** | lexer, parser (preprocessor), dict, string pool, hash index, decl manager with basic decls, lang, mapfile, manifest, binary reader, math, `--self-test` | every decl in `_ordered` tokenizes with 0 errors; `printEntityDef monster_demon_imp`; `mapinfo game/mars_city1`; lang lookup; self-test passes |
| M2 Session, input, saves | **done 2026-10-31** | session states, map change calling stub hooks, level/frame arenas, usercmd (keyboard, mouse, gamepad) with a per-player ring, bit message read/write, in-window console, savefile container + async writer, screenshots, jobs phase A | `map game/mars_city1` mounts the map container and calls hooks in order; save/load round trip; Esc to title; `error` leaves tag counters unchanged |
| M3 GPU and 2D | next | gpu device/targets, image (`.bimage`), material, vertex cache, 2D GUI model, font | `.bimage` drawn via DrawStretchPic; console text via `newfonts`; stencil mask; `_currentRender` capture |
| M4 Static world and lights (+G0) | | world (`.bproc`), portals, frontend, interactions, stencil shadows, fog/blend lights; game spawn without script, collision, clip, PVS | `map game/mars_city1` spawns every entity, renders lit and shadowed static world matching a BFG reference |
| M5 Models and animation | | MD5 + GPU skinning, particles, deforms, decals, subviews, in-world GUIs | imp animates lit and shadowed; mirror and monitors render |
| M6 Player, script, weapons | | player physics, movers, triggers, items, SWF HUD, old GUI, script VM, weapons, projectiles | walk through mars_city1 + admin; every `.script` compiles; weapon state machines run; weapon shadows on |
| M7 AI and AF physics | | AI, AAS, monster physics, IK, ragdolls, grabber | monsters navigate and die; stable ragdolls |
| M8 Sound and render polish | | mixer, `.idwav`, MS-ADPCM, occlusion; resolution scale, gamma, screenshots, debug tools | spatial audio; `classic_visuals.cfg` restores 2004 balance |
| M9 Saves, menus, threads | | save/restore everywhere, shell/PDA menus, transitions, job threads, `com_smp` | quickload mid-combat restores state; threaded frame stable |
| M10 Campaign and classic options | | all three campaigns completable; classic flashlight/ammo/movement options | playthrough checklist |
| M11 Co-op | | `rgd3_net` over SDL3_net, bit messages, snapshots + delta compression, client prediction, co-op session and rules, `GetLocalPlayer` audit closed | two machines finish mars_city1 → admin together (doors, GUIs, cinematics, transition); a client joins mid-level; a dead player respawns; 4 players stay in sync under 100 ms simulated lag and 2% packet loss; one RoE level with the artifact (Helltime) and the grabber and one Lost Mission level with its transition finish in co-op; every `IsMultiplayer`/`GetLocalClientNum` site classified |
| M12 Enhanced lighting | | shadow-map mode switchable against Classic | soft shadows at a measured cost; Classic output unchanged |

## Explicitly unfinished

- Everything after M2 in the table above.
- The game, renderer and sound system are stubs (`src/rgd3_game_stub.c`,
  `rgd3_render_stub.c`, `rgd3_sound_stub.c`). The game stub parses the map's
  entities, keeps a 60 Hz game clock, consumes usercmds, saves and restores a
  small payload, and has an invisible pause "shell" toggled by Escape. No
  screen wipes, loading GUI or main-menu graphics yet (renderer and SWF).
- The console draws with the rg_text demo font on the swapchain; it moves
  to the renderer's 2D path and the game's own fonts in M3.
- Saves: no thumbnail yet; save details use only the English date format.
- Leaving a map releases its resource container and level memory at once;
  the original keeps them until the next level load.
- The usercmd mouse path ignores SDL key repeats; the console repeats its
  own held keys.
- Decl types without a parser yet use the default "skip the braces" parse:
  material and table lookups in materials (M3), sound (M8), fx / particle /
  articulatedFigure (M5-M7), model (M5). Skin remapping (`RemapShaderBySkin`)
  waits for materials. `CacheDictionaryMedia` is a NULL hook until the game
  exists.
- The lexer "golden corpus" is a pinned CRC of this port's token stream over
  all 896 decl files, plus hand-written expected token streams in the lexer
  and parser tests. It is not cross-checked against id's C++ lexer: building
  that needs most of idlib.
- Writing `.map` files, decl text compression, and editor-only decl
  functions (CreateNewDecl, RenameDecl, ReplaceSourceFileText) are not
  ported.
- `rgd3_lexer_load_file` records the requested path as the file name, where
  the original records the full path. This only affects messages and the
  parser's include-relative-to-current-file attempt.
- `com_showFPS` defaults to 1 during bring-up; it returns to the original 0
  once the HUD exists.
- Out of scope: Bink video, deathmatch and every other competitive mode
  (and their maps, entities and menus), lobbies and matchmaking, Doom Classic,
  non-English sound packs, demos and AVI capture, editors and map tools,
  journaling, loading 2004 pk4 data or 2004 map lighting, enemy placement
  changes.

## Acceptance

### M0 (verified 2026-10-29, Windows 11, D3D12)

- `build.bat dev|debug|release|profile` each compile `src/all.c` in one `cl`
  call with zero warnings at `/W3`.
- `build_shaders.bat` compiles the rg_text shaders to DXIL, SPIR-V and MSL.
- `rgd3_dev.exe` with no arguments finds the Steam install, mounts
  `_common` (2262 entries), `_ordered` (1336), `_sound_pc` (4759) and
  `_sound_pc_en` (840), executes `default.cfg` (41 bindings), opens a
  1280x720 window on D3D12 and draws the FPS overlay.
- `+set fs_basepath "<install>"` overrides auto-detection.
- Through stdin: `listResources _common` prints 2262 rows with offsets;
  `dir materials .mtr` lists 128 files; `set`, `seta`, `bind`, `listBinds`,
  `listCvars`, `exec default.cfg`, `memTags` and `quit` work.
- With `developer 1`, `error <text>` drops back to the frame loop and later
  commands still run.
- Archived cvars and bindings are written to
  `%APPDATA%\superwendel\rgd3\base\rgd3.cfg` and read back on the next start.

### M1 (verified 2026-10-31)

- `rgd3_dev.exe --self-test` exits 0 in all four build modes: 105 checks
  (str, cmdargs, hash index, dict, bin, math 216, lexer 81, parser 96,
  dict parse, drops, lexer corpus, data), 0 failures. Leaks fail the run:
  any heap tag still allocated, or any object left in the unwind registry,
  after shutdown. The drop tests drop on purpose with a long-token lexer and
  a truncated map both in a called function and declared inside the guarded
  block, a parser with a 300-character undefined name in `#if`, and a map
  owned outside the stack (keeps its partial data for its owner), and check
  that tag counters and registry size are unchanged. A second review's
  optimized reproducer (same-function and `__forceinline` scopes, `/O2 /Ob1`)
  reports no leak after the switch to registration-order unwinding.
- Configs run default.cfg, then rgd3.cfg, then autoexec.cfg (appended as
  `Common.cpp` does; review 2026-10-31 found them running in reverse).
- All four build modes compile with zero warnings.
- `parseAllDecls` parses all 15667 decls in 896 files in about 90 ms. The 3
  defaulted tables and 4 warnings all come from typos in the shipped data
  (`docs/formats.md`); the original reports the same.
- `printEntityDef monster_demon_imp` prints the source and the dict with
  `monster_default` inherited (health 130).
- `mapinfo game/mars_city1` mounts `maps/mars_city1.resources` and parses
  `maps/game/mars_city1.map`: 2026 entities, 4133 brushes, 2678 patches,
  149 classes, all with an entityDef.
- `lang #str_02990` prints "Sub Level 9" (9104 strings from
  `strings/english.lang`).
- `printPreload game/mars_city1` reads 4053 entries.
- The parser (tested separately on the Steam data) preprocesses all 127
  `guis/*.gui` and `script/*.script` files with no errors.
- The M0 console checks still pass with the real lexer; list filters such
  as `listCvars fs_*` and `listResources _common *.mtr` work.

### M2 (verified 2026-10-31)

- `--self-test` exits 0 in all four build modes: 185 checks, 0 failures, no
  leaked heap tag or unwind entry. New in M2: usercmd 101, console 65,
  bitmsg 99, savegame 159 (each counted as one check in the total), jobs,
  and a session section that drives the real frame loop:
  - `map game/mars_city1` puts the session in LOADING; the next frame runs
    ExecuteMapChange and the subsystem calls come in `Common_load.cpp`'s
    order (game_map_shutdown, render/sound begin level load, preload x3,
    render world init, persistent info, server info, init from new map,
    shell menu, end level load x2, ..., generate interactions);
  - 60 Hz tics against real time (13-17 tics in 250 ms);
  - `saveGame` / play on / `loadGame` returns the game to the saved time
    through `game_init_from_save_game`;
  - Escape opens the pause menu and stops tics, Escape again resumes;
  - `disconnect`, an error drop in a running game and a missing map all
    return to IDLE with the same heap block counts as before the map.
- Live, through stdin: `map game/mars_city1`, `saveGame m2check`,
  `loadGame m2check` restores the game time; `testSavegameEnumerate` lists
  the slots; `screenshot` writes a PNG of the window (status line, console).
- `wait N` waits N frames: text-console lines are released one per frame, as
  `Sys_ConsoleInput` does, and the wait counter drops once per frame (the
  original drops it on every command-buffer run, i.e. per input event).
- Review fixes (2026-10-31, each with a regression check in the "review"
  self-test section): no save without a running game; a slot is replaced
  all or nothing (old files moved to `.bak`, restored on any failure; the
  check fails against the old loop); focus loss drops queued key events
  before clearing input; rebinding clears usercmd state (`SetBinding`);
  switching or losing the active pad releases its input; INI values are
  unquoted once, so quotes survive; `.tmp`/`.bak` file names are reserved;
  the console edit field fits the window width; failed PNG writes are
  reported.
- Second review round (2026-10-31; each check confirmed to fail against the
  previous code): INI values are read as raw bytes after `=`, so UTF-8
  survives (the lexer's rest-of-line blanks bytes >= 0x80); focus loss also
  drops queued mouse/pad events, the mouse accumulator and the pads'
  buffered events; focus gain and a change of active pad post the pad's
  held buttons and axes (`rgd3_gamepad_resync`), tested with two SDL
  virtual gamepads; the edit field clamps scroll to the cursor whenever it
  is drawn (narrowed console, recalled long lines, Ctrl-E); screenshot
  encoding is tested against a file that rejects writes. 185 checks.
- Configs run default.cfg, rgd3.cfg, then autoexec.cfg.

### Later milestones

Exit criteria in the Milestones table, plus `rgd3_dev.exe --self-test` once it
exists (M1), reference screenshots from BFG retail for M4/M5, and a campaign
checklist for M10.
