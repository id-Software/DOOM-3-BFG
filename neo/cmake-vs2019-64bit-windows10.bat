cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 16" -A x64 -DCMAKE_INSTALL_PREFIX=../bin/windows10-64 -DWINDOWS10=ON ../neo
pause