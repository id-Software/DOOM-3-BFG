cd ..
rm -rf xcode-vulkan-debug
mkdir xcode-vulkan-debug
cd xcode-vulkan-debug
# note 1: remove or set -DCMAKE_SUPPRESS_REGENERATION=OFF to reenable ZERO_CHECK target which checks for CMakeLists.txt changes and re-runs CMake before builds
#         however, if ZERO_CHECK is reenabled **must** add VULKAN_SDK location to Xcode Custom Paths (under Prefs/Locations) otherwise build failures may occur
# note 2: policy CMAKE_POLICY_DEFAULT_CMP0142=NEW suppresses non-existant per-config suffixes on Xcode library search paths, works for cmake version 3.25 and later
#note 3: env variable MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1 enables MoltenVK's image view swizzle which may be required on older macOS versions or hardware (see vulkaninfo)
# note 4: env variable MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=2 enables MoltenVK's use of Metal argument buffers only if VK_EXT_descriptor_indexing is enabled
cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug -DUSE_VULKAN=ON -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_XCODE_SCHEME_ENVIRONMENT="MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1;MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=2" -DCMAKE_SUPPRESS_REGENERATION=ON -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -DCMAKE_POLICY_DEFAULT_CMP0142=NEW -Wno-dev
