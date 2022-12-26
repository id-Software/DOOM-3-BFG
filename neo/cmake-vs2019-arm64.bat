cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 16" -A arm64 -DUSE_INTRINSICS_SSE=OFF -DFFMPEG=OFF -DBINKDEC=ON ../neo 
pause