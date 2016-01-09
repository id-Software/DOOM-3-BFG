astyle.exe -v --formatted --options=astyle-options.ini --exclude="libs" --recursive *.h
astyle.exe -v --formatted --options=astyle-options.ini --exclude="libs" --exclude="d3xp/gamesys/SysCvar.cpp" --exclude="d3xp/gamesys/Callbacks.cpp" --exclude="sys/win32/win_cpu.cpp" --exclude="sys/win32/win_main.cpp" --recursive *.cpp
astyle.exe -v -Q --options=astyle-options.ini ../base/renderprogs/postprocess.pixel
astyle.exe -v -Q --options=astyle-options.ini ../base/renderprogs/AmbientOcclusion_AO.pixel
astyle.exe -v -Q --options=astyle-options.ini ../base/renderprogs/AmbientOcclusion_blur.pixel
astyle.exe -v -Q --options=astyle-options.ini ../base/renderprogs/AmbientOcclusion_minify.pixel
astyle.exe -v -Q --options=astyle-options.ini ../base/renderprogs/DeepGBufferRadiosity_DeepGBufferRadiosity.pixel

pause