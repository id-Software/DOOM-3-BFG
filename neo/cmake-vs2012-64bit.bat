cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 11 Win64" -DCMAKE_INSTALL_PREFIX=../bin/win8-64 ../neo
pause