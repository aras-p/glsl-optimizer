rd /Q /S cmake_temp
mkdir cmake_temp
cd cmake_temp

cmake -G "Visual Studio 10" -DCMAKE_INSTALL_PREFIX=. ../
