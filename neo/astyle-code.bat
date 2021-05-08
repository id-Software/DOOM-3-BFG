astyle.exe -v --formatted --options=astyle-options.ini --exclude="libs" --exclude="extern" --recursive *.h
astyle.exe -v --formatted --options=astyle-options.ini --exclude="libs" --exclude="extern" --exclude="d3xp/gamesys/SysCvar.cpp" --exclude="d3xp/gamesys/Callbacks.cpp" --exclude="sys/win32/win_cpu.cpp" --recursive *.cpp

astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/imgui/*.h
astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/imgui/*.cpp

astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/stb/*.h
astyle.exe -v --formatted --options=astyle-options.ini --recursive libs/tinyexr/*.h

astyle.exe -v -Q --options=astyle-options.ini --recursive ../base/renderprogs/*.hlsl

pause