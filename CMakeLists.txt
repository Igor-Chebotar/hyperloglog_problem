cmake_minimum_required(VERSION 3.14)
project(hyperloglog_research CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

add_executable(main
    src/main.cpp
    src/random_stream_gen.cpp
    src/hash_func_gen.cpp
    src/hyperloglog.cpp
    src/hyperloglog_improved.cpp
    src/exact_counter.cpp
)

target_include_directories(main PRIVATE include)
