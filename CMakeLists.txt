cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


project(adam VERSION 0.1.0 DESCRIPTION "ADAM is a CSPRNG inspired by ISAAC64 and ChaCha20")
set(TAGS "core csprng cryptography pseudorandom generator secure cipher hash scrambler")

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib)

add_executable(adam main.cc adam.cc)
target_link_libraries(adam PRIVATE pkcore)

configure_file(adam.h.in ${CMAKE_SOURCE_DIR}/include/adam.h)

set(ADAM_TITLE "#5821db")
set(ADAM_BOLD "#e8334e")
configure_file(README.md ${CMAKE_SOURCE_DIR}/help/adam.md)
