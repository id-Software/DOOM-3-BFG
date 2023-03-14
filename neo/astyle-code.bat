astyle.exe -v --formatted --options=astyle-options.ini --exclude="libs" --exclude="extern" --recursive *.h
astyle.exe -v --formatted --options=astyle-options.ini --exclude="libs" --exclude="extern" --exclude="d3xp/gamesys/SysCvar.cpp" --exclude="d3xp/gamesys/Callbacks.cpp" --exclude="sys/win32/win_cpu.cpp" --recursive *.cpp

astyle.exe -v --formatted --options=astyle-options.ini --recursive ../doomclassic/*.h
astyle.exe -v --formatted --options=astyle-options.ini --recursive ../doomclassic/*.cpp

REM astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/imgui/*.h
REM astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/imgui/*.cpp

REM astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/stb/*.h
REM astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/tinyexr/*.h

REM astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/oggvorbis/*.h
REM astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/oggvorbis/*.c

astyle.exe -v -Q --options=astyle-options.ini --recursive shaders/*.hlsl

pause