cd ..
rm -rf xcode-vulkan-universal
mkdir xcode-vulkan-universal
cd xcode-vulkan-universal
# remove or set -DCMAKE_SUPPRESS_REGENERATION=OFF to reenable ZERO_CHECK target which checks for CMakeLists.txt changes and re-runs CMake before builds
# however, if ZERO_CHECK is reenabled **must** add VULKAN_SDK location to Xcode Custom Paths (under Prefs/Locations) otherwise build failures may occur
# note: universal openal-soft library and include paths assume MacPorts install locations
cmake -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_CONFIGURATION_TYPES="Release;MinSizeRel;RelWithDebInfo" -DSDL2=ON -DFFMPEG=OFF -DBINKDEC=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF -DUSE_MoltenVK=ON -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_SUPPRESS_REGENERATION=ON -DOPENAL_LIBRARY=/opt/local/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/opt/local/include ../neo -Wno-dev
