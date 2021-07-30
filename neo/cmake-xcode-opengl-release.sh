cd ..
rm -rf xcode-opengl-release
mkdir xcode-opengl-release
cd xcode-opengl-release
cmake -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES="Release;MinSizeRel;RelWithDebInfo" -DSDL2=ON -DFFMPEG=OFF -DBINKDEC=ON -DCMAKE_XCODE_GENERATE_SCHEME=ON -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
