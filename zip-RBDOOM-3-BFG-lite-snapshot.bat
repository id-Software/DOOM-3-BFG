REM 7z a RBDOOM-3-BFG-1.3.1.1-lite-win64-20220109-git-xxxxxxx.7z -r base/env/ base/maps/*.lightgrid base/maps/*_extra_ents.map -x!generated
set filename=RBDOOM-3-BFG-1.5.0.x-lite-win64-yyyymmdd-git-xxxxxxx.7z
7z a %filename% README.md RELEASE-NOTES.md base/devtools.cfg base/modelviewer.cfg base/extract_resources.cfg base/convert_maps_to_valve220.cfg base/def/*.def base/materials/*.mtr base/textures/common base/textures/editor base/maps/zoomaps -x!generated -xr!autosave -xr!*.xcf -xr!*.blend
7z a %filename% -r base/renderprogs2/dxil/*.bin
7z a %filename% -r base/renderprogs2/spirv/*.bin
7z a %filename% base/_tb/fgd/*.fgd
7z a %filename% tools/trenchbroom -xr!TrenchBroom-nomanual* -xr!TrenchBroom.pdb
pause
