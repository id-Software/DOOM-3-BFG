cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 11" ../neo
pause