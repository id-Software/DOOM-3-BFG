cd ..
rm -rf build
mkdir build
cd build

# asemarafa/SRS - Determine the Homebrew path prefix for openal-soft
if [ -z "$OPENAL_PREFIX" ]; then
  OPENAL_PREFIX=$(brew --prefix openal-soft 2>/dev/null)
  if [ -z "$OPENAL_PREFIX" ]; then
  	echo "Error: openal-soft is not installed via Homebrew."
  	echo "Either install it using 'brew install openal-soft' or define the path prefix via OPENAL_PREFIX."
  	exit 1
  fi
fi

# change or remove -DCMAKE_OSX_DEPLOYMENT_TARGET=<version> to match supported runtime targets
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS_RELEASE="-DNDEBUG -DID_RETAIL" -DCMAKE_OSX_DEPLOYMENT_TARGET=12.1 -DFFMPEG=OFF -DBINKDEC=ON -DUSE_MoltenVK=ON -DOPENAL_LIBRARY=$OPENAL_PREFIX/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=$OPENAL_PREFIX/include ../neo -Wno-dev
