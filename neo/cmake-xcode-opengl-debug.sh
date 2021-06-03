cd ..
rm -rf xcode-opengl-debug
mkdir xcode-opengl-debug
cd xcode-opengl-debug
cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug -DSDL2=ON -DCMAKE_XCODE_GENERATE_SCHEME=ON -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
