cmake_minimum_required(VERSION 3.21)
project(zenoh_example)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Put runtime outputs in build/bin ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# --- Dependencies ---
find_package(zenohc REQUIRED)
find_package(zenohcxx REQUIRED)  # header-only shim targeting zenohc

# --- Library ---
add_library(ZNode SHARED
  src/node.cpp                 # ensure exact file names/case exist
  src/zenoh_unity_wrapper.cpp
)
target_include_directories(ZNode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ZNode PUBLIC zenohcxx::zenohc)

# Windows-only conveniences
if(WIN32)
  set_target_properties(ZNode PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

  # Copy dependent DLLs next to the targets (Windows only)
  foreach(tgt IN ITEMS ZNode)
    add_custom_command(TARGET ${tgt} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_RUNTIME_DLLS:${tgt}> $<TARGET_FILE_DIR:${tgt}>
      COMMAND_EXPAND_LISTS)
  endforeach()
endif()

# --- Executables ---
add_executable(publisher  src/publisher.cpp)
add_executable(subscriber src/subscriber.cpp)
target_link_libraries(publisher  PUBLIC ZNode)
target_link_libraries(subscriber PUBLIC ZNode)

# On Linux, make the binaries find libZNode.so next to themselves
if(UNIX AND NOT APPLE)
  foreach(tgt IN ITEMS publisher subscriber)
    set_target_properties(${tgt} PROPERTIES
      BUILD_RPATH "\$ORIGIN"
      INSTALL_RPATH "\$ORIGIN"
    )
  endforeach()
endif()
