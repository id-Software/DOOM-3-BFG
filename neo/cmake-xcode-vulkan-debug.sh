cd ..
rm -rf xcode-vulkan-debug
mkdir xcode-vulkan-debug
cd xcode-vulkan-debug
cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug -DSDL2=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_XCODE_SCHEME_ENVIRONMENT="MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1" -DCMAKE_SUPPRESS_REGENERATION=ON -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
