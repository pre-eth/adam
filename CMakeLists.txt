cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-O3 -march=native")

set(EXECUTABLE_OUTPUT_PATH "~/.local/bin")

project(adam VERSION 0.1.0 DESCRIPTION "ADAM is a CSPRNG inspired by ISAAC64 and ChaCha20")
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(adam src/main.cc src/adam.cc src/csprng.cc src/cmd.cc)

configure_file(${CMAKE_SOURCE_DIR}/include/adam.h.in  ${CMAKE_SOURCE_DIR}/include/adam.h)