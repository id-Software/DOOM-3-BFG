cd ..
rm -rf build-vulkan
mkdir build-vulkan
cd build-vulkan
# change or remove -DCMAKE_OSX_DEPLOYMENT_TARGET=<version> to match supported runtime targets
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=10.12 -DSDL2=ON -DFFMPEG=OFF -DBINKDEC=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF -DUSE_MoltenVK=ON -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
