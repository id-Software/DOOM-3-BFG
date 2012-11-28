astyle.exe -v --options=astyle-options.ini --exclude="libs" --recursive *.h
astyle.exe -v --options=astyle-options.ini --exclude="libs" --exclude="idlib/math/Simd.cpp" --exclude="game/gamesys/SysCvar.cpp" --exclude="game/gamesys/Callbacks.cpp" --exclude="sys/win32/win_cpu.cpp" --exclude="sys/win32/win_main.cpp" --exclude="sys/win32/win_shared.cpp"  --recursive *.cpp

pause