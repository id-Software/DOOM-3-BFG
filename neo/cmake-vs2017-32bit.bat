cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 15" -DWINDOWS10=OFF ../neo
pause