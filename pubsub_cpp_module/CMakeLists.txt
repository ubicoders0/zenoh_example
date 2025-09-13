cmake_minimum_required(VERSION 3.21)   # needed for TARGET_RUNTIME_DLLS
project(zenoh_example)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Put runtime outputs in build/bin (works with VS multi-config) ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} ${CMAKE_BINARY_DIR}/bin)
endforeach()  # RUNTIME_OUTPUT_DIRECTORY_<CONFIG> is honored by VS. :contentReference[oaicite:0]{index=0}

# --- Dependencies ---
find_package(zenohc REQUIRED)
find_package(zenohcxx REQUIRED)  # header-only C++ shim targeting zenohc

# --- Your Node library (DLL) ---
add_library(Node SHARED
  src/Node.cpp
  src/zenoh_unity_wrapper.cpp
  # include/Node.h
)
target_include_directories(Node PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(Node PUBLIC zenohcxx::zenohc)  # pulls in zenohc
# Export public symbols automatically on Windows (handy for quick DLLs)
set_target_properties(Node PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)  # :contentReference[oaicite:1]{index=1}

# --- Executables ---
add_executable(publisher  src/publisher.cpp)
add_executable(subscriber src/subscriber.cpp)
target_link_libraries(publisher  PUBLIC Node)      # link via your DLL
target_link_libraries(subscriber PUBLIC Node)

# --- After build: copy all runtime DLL deps next to each target (incl. zenohc.dll) ---
# TARGET_RUNTIME_DLLS enumerates runtime .dll dependencies of a target (CMake ≥3.21).
# COMMAND_EXPAND_LISTS expands the list for copy_if_different. :contentReference[oaicite:2]{index=2}
foreach(tgt IN ITEMS Node publisher subscriber)
  add_custom_command(TARGET ${tgt} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${tgt}> $<TARGET_FILE_DIR:${tgt}>
    COMMAND_EXPAND_LISTS)
endforeach()

