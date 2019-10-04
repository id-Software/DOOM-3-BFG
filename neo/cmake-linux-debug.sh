cd ..
rm -rf build
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DSDL2=ON -DOpenGL_GL_PREFERENCE=GLVND ../neo
