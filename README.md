# rgd3

A rewrite of the DOOM 3 BFG Edition engine in C11 on SDL3 GPU and the rg_*
libraries. It loads the shipped BFG game data unchanged and targets the
single-player campaigns, then co-op through them over SDL3_net. The original id
Software source is kept under `neo/` as reference; see `README.txt` and
`COPYING.txt` for its license.

Status: milestones M0 (engine skeleton), M1 (text, decls, maps, math) and M2 (session, map loading, input, console, saves) done. See `PLAN.md`.

## Requirements

- Windows x64, Visual Studio 2022 (MSVC).
- SDL 3.4.10 at `C:\libs\SDL3-3.4.10`, or set `SDL3_DIR`.
- SDL_shadercross 3.0.0 at `C:\libs\SDL3_shadercross-3.0.0-windows-VC-x64`, or
  set `SHADERCROSS` to `shadercross.exe`.
- `rg_core` and `rg_text` checked out next to this repository, or set
  `RG_CORE_DIR` / `RG_TEXT_DIR`.
- DOOM 3 BFG Edition installed through Steam, or pass
  `+set fs_basepath "<install dir>"`.

## Build and run

```
build_shaders.bat
build.bat dev
rgd3_dev.exe
```

Console commands are read from stdin while the window runs, for example
`listResources _common`, `dir materials .mtr`, `listCvars`, `quit`.
