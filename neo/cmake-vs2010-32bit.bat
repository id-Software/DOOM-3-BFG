cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 10" -DCMAKE_INSTALL_PREFIX=../bin/win32 ../neo
pause