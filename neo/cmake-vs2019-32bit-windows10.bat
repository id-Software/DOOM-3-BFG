cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 16" -A Win32 -DWINDOWS10=ON ../neo
pause