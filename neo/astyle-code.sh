#!/bin/sh

# TODO make sure that the astyle version is >= 2.03 or quit

astyle -v --formatted --options=astyle-options.ini --exclude="libs" --recursive *.h
astyle -v --formatted --options=astyle-options.ini --exclude="libs" --exclude="d3xp/gamesys/SysCvar.cpp" --exclude="d3xp/gamesys/Callbacks.cpp" --exclude="sys/win32/win_cpu.cpp" --exclude="sys/win32/win_main.cpp" --recursive *.cpp

#pause