cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 16" -A arm64 -D WINDOWS10=ON -D USE_INTRINSICS_SSE=OFF -D FFMPEG=OFF -D BINKDEC=ON ../neo 
pause