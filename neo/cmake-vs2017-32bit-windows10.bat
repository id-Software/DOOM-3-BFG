cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 15" -DCMAKE_INSTALL_PREFIX=../bin/windows10-32 -DWINDOWS10=ON ../neo
pause