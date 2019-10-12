cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 16" -A x64 -DCMAKE_INSTALL_PREFIX=../bin/windows7-64 ../neo
pause