cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-O3 -march=native")

project(adam VERSION 0.1.0 DESCRIPTION "ADAM is a CSPRNG inspired by ISAAC64 and ChaCha20")
include_directories(${CMAKE_SOURCE_DIR}/include)

file(READ ${CMAKE_SOURCE_DIR}/chunk GENESIS_CHUNK)
configure_file(csprng.cc ${CMAKE_SOURCE_DIR}/csprng.cc)
file(REMOVE ${CMAKE_SOURCE_DIR}/chunk ${CMAKE_SOURCE_DIR}/gen)

add_executable(adam main.cc adam.cc csprng.cc cmd.cc)

configure_file(${CMAKE_SOURCE_DIR}/include/adam.h.in  ${CMAKE_SOURCE_DIR}/include/adam.h)
set(ADAM_TITLE "#5821db")
set(ADAM_BOLD "#e8334e")
configure_file(adam.md  ${CMAKE_SOURCE_DIR}/README.md)
