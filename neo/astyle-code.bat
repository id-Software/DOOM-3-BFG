astyle.exe -v --options=astyle-options.ini --exclude="libs" --exclude="framework/minizip" --recursive *.h
astyle.exe -v --options=astyle-options.ini --exclude="libs" --exclude="framework/minizip" --exclude="idlib/math/Simd.cpp" --exclude="d3xp/gamesys/SysCvar.cpp" --exclude="d3xp/gamesys/Callbacks.cpp" --exclude="sys/win32/win_cpu.cpp" --exclude="sys/win32/win_main.cpp" --recursive *.cpp

pause