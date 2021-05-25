cd ..
rm -rf xcode-vulkan-release
mkdir xcode-vulkan-release
cd xcode-vulkan-release
cmake -G Xcode -DCMAKE_SUPPRESS_REGENERATION=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=10.12 -DSDL2=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF -DUSE_MoltenVK=ON -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
