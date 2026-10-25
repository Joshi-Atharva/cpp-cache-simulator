cmake_minimum_required(VERSION 3.16)
project(CacheLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cache_core
    src/Cache.cpp
    src/TraceParser.cpp
)
target_include_directories(cache_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(cache_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)

add_executable(cache_sim src/main.cpp)
target_link_libraries(cache_sim PRIVATE cache_core)
target_compile_options(cache_sim PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)

add_executable(cache_tests tests/test_cache.cpp)
target_link_libraries(cache_tests PRIVATE cache_core)
target_compile_options(cache_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)

enable_testing()
add_test(NAME cache_tests COMMAND cache_tests)
