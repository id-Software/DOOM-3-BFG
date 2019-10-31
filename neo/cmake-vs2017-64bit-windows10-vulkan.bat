cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 15 Win64" -DWINDOWS10=ON -DUSE_VULKAN=ON -DUSE_FFMPEG=ON ../neo
pause