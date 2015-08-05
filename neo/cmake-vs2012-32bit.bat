cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 11" -DCMAKE_INSTALL_PREFIX=../bin/win8-32 ../neo
pause