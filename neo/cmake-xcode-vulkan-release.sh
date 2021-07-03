cd ..
rm -rf xcode-vulkan-release
mkdir xcode-vulkan-release
cd xcode-vulkan-release
cmake -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES="Release;MinSizeRel;RelWithDebInfo" -DSDL2=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF -DUSE_MoltenVK=ON -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_SUPPRESS_REGENERATION=ON -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
