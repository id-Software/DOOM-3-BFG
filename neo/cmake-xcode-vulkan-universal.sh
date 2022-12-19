cd ..
rm -rf xcode-vulkan-universal
mkdir xcode-vulkan-universal
cd xcode-vulkan-universal
# note 1: remove or set -DCMAKE_SUPPRESS_REGENERATION=OFF to reenable ZERO_CHECK target which checks for CMakeLists.txt changes and re-runs CMake before builds
#         however, if ZERO_CHECK is reenabled **must** add VULKAN_SDK location to Xcode Custom Paths (under Prefs/Locations) otherwise build failures may occur
# note 2: policy CMAKE_POLICY_DEFAULT_CMP0142=NEW suppresses non-existant per-config suffixes on Xcode library search paths, works for cmake version 3.25 and later
# note 3: universal openal-soft library and include paths assume MacPorts install locations
cmake -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_CONFIGURATION_TYPES="Release;MinSizeRel;RelWithDebInfo" -DFFMPEG=OFF -DBINKDEC=ON -DUSE_VULKAN=ON -DUSE_MoltenVK=ON -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_XCODE_SCHEME_ENABLE_GPU_API_VALIDATION=OFF -DCMAKE_SUPPRESS_REGENERATION=ON -DOPENAL_LIBRARY=/opt/local/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/opt/local/include ../neo -DCMAKE_POLICY_DEFAULT_CMP0142=NEW -Wno-dev
