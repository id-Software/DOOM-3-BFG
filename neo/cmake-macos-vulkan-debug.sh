cd ..
rm -rf build
mkdir build
cd build
cmake -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DSDL2=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF ../neo -Wno-dev -Wexpansion-to-defined