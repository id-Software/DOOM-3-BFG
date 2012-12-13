cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 10 Win64" ../neo
pause