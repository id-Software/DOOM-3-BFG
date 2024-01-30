cd ..
rm -rf xcode-debug
mkdir xcode-debug
cd xcode-debug

# asemarafa/SRS - Determine the Homebrew path prefix for openal-soft
if [ -z "$OPENAL_PREFIX" ]; then
  OPENAL_PREFIX=$(brew --prefix openal-soft 2>/dev/null)
  if [ -z "$OPENAL_PREFIX" ]; then
  	echo "Error: openal-soft is not installed via Homebrew."
  	echo "Either install it using 'brew install openal-soft' or define the path prefix via OPENAL_PREFIX."
  	exit 1
  fi
fi

# note 1: remove or set -DCMAKE_SUPPRESS_REGENERATION=OFF to reenable ZERO_CHECK target which checks for CMakeLists.txt changes and re-runs CMake before builds
#         however, if ZERO_CHECK is reenabled **must** add VULKAN_SDK location to Xcode Custom Paths (under Prefs/Locations) otherwise build failures may occur
# note 2: policy CMAKE_POLICY_DEFAULT_CMP0142=NEW suppresses non-existant per-config suffixes on Xcode library search paths, works for cmake version 3.25 and later
# note 3: env variable MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1 enables MoltenVK's image view swizzle which may be required on older macOS versions or hardware (see vulkaninfo) - only used for VulkanSDK < 1.3.275
# note 4: env variable MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=0 disables synchronous queue submits which is optimal for the synchronization method used by the game - only used for VulkanSDK < 1.3.275
# note 5: env variable MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=2 enables MoltenVK's use of Metal argument buffers only if VK_EXT_descriptor_indexing is enabled - only used for VulkanSDK < 1.3.275
# note 6: env variable MVK_CONFIG_TIMESTAMP_PERIOD_LOWPASS_ALPHA=1.0 disables MoltenVK's timestampPeriod lowpass filter for non-Apple GPUs - only used for VulkanSDK < 1.3.275
cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_XCODE_SCHEME_ENVIRONMENT="MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1;MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=0;MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=2;MVK_CONFIG_TIMESTAMP_PERIOD_LOWPASS_ALPHA=1.0" -DCMAKE_XCODE_SCHEME_ENABLE_GPU_API_VALIDATION=OFF -DCMAKE_SUPPRESS_REGENERATION=ON -DOPENAL_LIBRARY=$OPENAL_PREFIX/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=$OPENAL_PREFIX/include ../neo -DCMAKE_POLICY_DEFAULT_CMP0142=NEW -Wno-dev
