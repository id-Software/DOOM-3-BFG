REM 7z a RBDOOM-3-BFG-1.3.1.1-lite-win64-20220109-git-xxxxxxx.7z -r base/env/ base/maps/*.lightgrid base/maps/*_extra_ents.map -x!generated
7z a RBDOOM-3-BFG-1.3.1.1-lite-win64-20220109-git-xxxxxxx.7z README.md RELEASE-NOTES.md base/devtools.cfg base/modelviewer.cfg base/extract_resources.cfg base/convert_maps_to_valve220.cfg base/def/*.def base/_tb -x!generated
pause
