cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 15 Win64" ../neo
pause