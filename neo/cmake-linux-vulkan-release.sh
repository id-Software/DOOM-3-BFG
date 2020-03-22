cd ..
rm -rf build
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DSDL2=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF ../neo
