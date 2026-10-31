# BFG data formats used by rgd3

Verified against the Steam install of DOOM 3 BFG Edition on 2026-10-29.

## `.resources` containers

| Field | Type | Notes |
|---|---|---|
| magic | u32 big-endian | `0xD000000D` |
| table offset | u32 big-endian | |
| table length | u32 big-endian | |
| data | bytes | starts at offset 12, uncompressed |

Table, at `table offset`:

| Field | Type | Notes |
|---|---|---|
| entry count | u32 big-endian | |
| per entry: name length | u32 **little-endian** | `idFile::ReadString` uses `ReadInt` |
| per entry: name | bytes, no NUL | mixed case, lowercased and `\` to `/` at mount |
| per entry: offset | u32 big-endian | absolute file offset |
| per entry: length | u32 big-endian | |

Rules:

- Every `<basepath>/base/*.resources` is mounted at startup, sorted by name:
  `_common` (2262 entries), `_ordered` (1336), `_sound_pc` (4759),
  `_sound_pc_en` (840). Other languages ship only `.crc` files.
- Lookups search the most recently mounted container first. Within one
  container, a later duplicate name wins.
- `_ordered.resources` holds the startup decls in load order. The original
  reads it into memory and unmounts it after init; rgd3 does the same.
- Each map has `maps/<map>.resources`, mounted at level load (M2). Map names
  look like `game/mars_city1`; the container is `maps/mars_city1.resources`.
- With `fs_resourceLoadPriority 1` (default) containers are searched before
  loose files. `.crc` files are ignored.

## Decl files (verified 2026-10-31)

Text, read with the decl lexer flags (no string concatenation, no escapes,
path names, multi-character literals, `\` string continuation, non-fatal
errors). Each decl is `[type] <name> { ... }`; the type keyword may be
omitted when the folder has a default type.

| Folder | Extension | Default type |
|---|---|---|
| `materials` | `.mtr` | material |
| `skins` | `.skin` | skin |
| `sound` | `.sndshd` | sound |
| `def` | `.def` | entityDef |
| `fx` | `.fx` | fx |
| `particles` | `.prt` | particle |
| `af` | `.af` | articulatedFigure |
| `newpdas` | `.pda` | pda |

The shipped data holds 15667 decls in 896 files. The decls that don't parse
come from typos in id's files, which the original reports the same way:

- `table lavaflicker3` (`..5`)
- `table fivestage` (no inner braces)
- `table blood2` (trailing comma)
- a duplicate `light_color` in `monster_hunter_invul`

## `strings/*.lang`

UTF-8 with BOM. `// comments`, then `"key" "value"` pairs, one per line, up to
`}`. The value runs to the end of the line: unescaped quotes are dropped,
`\n \t \" \` are escapes, `\r` is ignored. Keys start with `#str_`; a value
that is itself a key redirects (at most two levels). BFG ships
`english.lang` only (9104 strings).

## `maps/<map>.preload`

| Field | Type |
|---|---|
| entry count | i32 big-endian |
| manifest name | i32 LE length + bytes |
| per entry: type | i32 BE (0 image, 1 model, 2 sample, 3 anim, 4 collision, 5 particle) |
| per entry: resource name | i32 LE length + bytes |
| per entry: filter, repeat, usage, cubeMap | 4 × i32 BE (image preload data) |

The name drops the `game/` folder: `game/mars_city1` → `maps/mars_city1.preload`
(in `_common`). mars_city1 lists 4053 entries.

## `.map`

Text, `Version 2`, entities `{ "key" "value" ... { brushDef3 / patchDef2 /
patchDef3 ... } }`, lexed with no string concatenation, no escapes and
path names. Parse errors are fatal (no `LEXFL_NOFATALERRORS`). The map is
inside the per-map container: `maps/mars_city1.resources` holds
`maps/game/mars_city1.map` (2026 entities, 4133 brushes, 2678 patches,
149 spawn classes, all with entityDefs; geometry CRC 0x8b89b51c).
