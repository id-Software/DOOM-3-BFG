cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 15" -DWINDOWS10=ON -DUSE_VULKAN=ON ../neo
pause