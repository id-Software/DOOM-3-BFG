cd ..
rm -rf xcode-opengl-universal
mkdir xcode-opengl-universal
cd xcode-opengl-universal
# note: universal openal-soft library and include paths assume MacPorts install locations
cmake -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_CONFIGURATION_TYPES="Release;MinSizeRel;RelWithDebInfo" -DSDL2=ON -DFFMPEG=OFF -DBINKDEC=ON -DCMAKE_XCODE_GENERATE_SCHEME=ON -DOPENAL_LIBRARY=/opt/local/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/opt/local/include ../neo -Wno-dev
