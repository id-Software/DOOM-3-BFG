cd ..
rm -rf build
mkdir build
cd build
# change or remove -DCMAKE_OSX_DEPLOYMENT_TARGET=<version> to match supported runtime targets
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS_RELEASE="-DNDEBUG -DID_RETAIL" -DCMAKE_OSX_DEPLOYMENT_TARGET=12.1 -DFFMPEG=OFF -DBINKDEC=ON -DUSE_MoltenVK=ON -DOPENAL_LIBRARY=/usr/local/opt/openal-soft/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/usr/local/opt/openal-soft/include ../neo -Wno-dev
