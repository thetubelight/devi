cmake_minimum_required(VERSION 3.16)
project(devi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# CPU-specific build (-march=native on GCC/Clang, /arch:AVX2 on MSVC): faster locally, but the binary may not run on other machines.
# Leave OFF for a binary you ship elsewhere (e.g. a Lichess bot host).
option(DEVI_NATIVE "Compile for this CPU (-march=native or /arch:AVX2)" OFF)

if(MSVC)
  add_compile_options(/W3 /O2 /EHsc /utf-8)
  # Statically link the MSVC runtime so devi.exe doesn't need the Visual C++ Redistributable installed.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  if(DEVI_NATIVE)
    add_compile_options(/arch:AVX2)
  endif()
else()
  add_compile_options(-Wall -Wextra)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
  if(DEVI_NATIVE)
    add_compile_options(-march=native)
  endif()
  if(MINGW)
    # MinGW-w64 (MSYS2 etc.): link libgcc/libstdc++/winpthreads statically -> no extra DLLs next to devi.exe.
    add_link_options(-static)
  endif()
endif()

find_package(Threads REQUIRED)

add_library(devi_core STATIC
  src/bitboard.cpp src/attacks.cpp src/zobrist.cpp src/position.cpp src/movegen.cpp
  src/perft.cpp src/eval.cpp src/tt.cpp src/search.cpp src/uci.cpp)
target_include_directories(devi_core PUBLIC src)
target_link_libraries(devi_core PUBLIC Threads::Threads)

add_executable(devi src/main.cpp)
target_link_libraries(devi PRIVATE devi_core)

enable_testing()
add_executable(perft_tests tests/perft_tests.cpp)
target_link_libraries(perft_tests PRIVATE devi_core)
add_executable(unit_tests tests/unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE devi_core)
add_test(NAME unit_tests COMMAND unit_tests)
add_test(NAME perft_quick COMMAND perft_tests)           # depth <= 5 on the standard positions
add_test(NAME bench COMMAND devi bench 5)                # smoke test: must run and exit 0
# Deep perft (depth 6-7) is opt-in:  ./perft_tests --deep
