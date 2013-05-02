cd "`dirname "$0"`"
rm -r -f cmake_temp
mkdir cmake_temp
cd cmake_temp
cmake -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_INSTALL_PREFIX=. -DCMAKE_BUILD_TYPE=DEBUG -G Xcode ../