cd ..
rm -rf xcode-vulkan-debug
mkdir xcode-vulkan-debug
cd xcode-vulkan-debug
cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug -DSDL2=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
