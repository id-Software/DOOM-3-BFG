cd ..
rm -rf xcode-vulkan-debug
mkdir xcode-vulkan-debug
cd xcode-vulkan-debug
# note 1: remove or set -DCMAKE_SUPPRESS_REGENERATION=OFF to reenable ZERO_CHECK target which checks for CMakeLists.txt changes and re-runs CMake before builds
#         however, if ZERO_CHECK is reenabled **must** add VULKAN_SDK location to Xcode Custom Paths (under Prefs/Locations) otherwise build failures may occur
# note 2: env variable MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1 enables MoltenVK's image view swizzle which may be required on older macOS versions or hardware (see vulkaninfo)
# note 3: env variable MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0 disables MoltenVK's use of Metal argument buffers, which then removes the limit on sampler allocation
# note 4: env variable VK_LAYER_MESSAGE_ID_FILTER=0xb408bc0b suppresses validation layer error messages caused by exceeding the maxSamplerAllocationCount limit
cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug -DSDL2=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_XCODE_SCHEME_ENVIRONMENT="MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1;MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0;VK_LAYER_MESSAGE_ID_FILTER=0xb408bc0b" -DCMAKE_SUPPRESS_REGENERATION=ON -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
