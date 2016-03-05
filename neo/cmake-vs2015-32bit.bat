cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 14" -DCMAKE_INSTALL_PREFIX=../bin/windows7-32 -DWINDOWS10=OFF ../neo
pause