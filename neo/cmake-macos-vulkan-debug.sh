cd ..
rm -rf build
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_DEPLOYMENT_TARGET=10.12 -DSDL2=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev