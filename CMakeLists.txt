cmake_minimum_required (VERSION 2.6)

project (mycc)

file (GLOB SOURCE_FILES src/*.c)

add_executable (mycc ${SOURCE_FILES})
