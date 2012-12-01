cd ..
del /s /q build
mkdir build
cd build
cmake -G "Eclipse CDT4 - MinGW Makefiles" ../neo
pause