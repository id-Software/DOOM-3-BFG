cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 10 Win64" -DCMAKE_INSTALL_PREFIX=../bin/win64 ../neo
pause