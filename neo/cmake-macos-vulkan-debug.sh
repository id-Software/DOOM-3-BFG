cd ..
rm -rf build
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DSDL2=ON -DUSE_VULKAN=ON -DSPIRV_SHADERC=OFF -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
echo "** NOTE: For this build type must set environment variable MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1 at runtime **"
