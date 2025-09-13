cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="C:/cmake_ext_libs/zenoh-cpp;C:/cmake_ext_libs/zenoh-c"
cmake --build build --config Release