cmake_minimum_required(VERSION 3.5)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Please use out of source build:
  cmake -Bbuild")
endif()

project(pngparam CXX)
add_executable(pngparam pngparam.cpp)
target_include_directories(pngparam PUBLIC ./rapidjson/include)
target_compile_features(pngparam PUBLIC cxx_std_23)
