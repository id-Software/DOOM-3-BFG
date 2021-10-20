7z a RBDOOM-3-BFG-1.3.0.39-baseSP_bakedlightdata.7z -r base/env/ base/maps/*.lightgrid base/maps/*_extra_ents.map -x!generated
REM sha256sum RBDOOM-3-BFG-1.3.0.39-base_bakedlightdata.7z >> SHA256SUMS.txt
pause
