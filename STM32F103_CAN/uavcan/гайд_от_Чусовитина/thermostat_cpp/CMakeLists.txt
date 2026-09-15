cmake_minimum_required(VERSION 3.25)
project(cyphal_thermostat C CXX)

include(FetchContent)
FetchContent_Declare(
  libcanard_source
  URL https://github.com/OpenCyphal/libcanard/archive/refs/tags/v4.0.0.zip
)
FetchContent_Declare(
  o1heap_source
  URL https://github.com/pavel-kirienko/o1heap/archive/refs/tags/3.0.0.zip
  SOURCE_SUBDIR .cmake_ignore
)

FetchContent_MakeAvailable(libcanard_source o1heap_source)

add_library(libcanard STATIC)
target_include_directories(libcanard
  PUBLIC
    ${libcanard_source_SOURCE_DIR}/libcanard
  PRIVATE
    ${libcanard_source_SOURCE_DIR}/lib/cavl2
)
target_sources(libcanard PRIVATE
  ${libcanard_source_SOURCE_DIR}/libcanard/canard.c
)

add_library(o1heap STATIC)
target_include_directories(o1heap
  PUBLIC
    ${o1heap_source_SOURCE_DIR}/o1heap
)
target_sources(o1heap PRIVATE
  ${o1heap_source_SOURCE_DIR}/o1heap/o1heap.c
)

add_library(uavcan INTERFACE)
target_include_directories(uavcan INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(nunavut INTERFACE)
target_include_directories(nunavut INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(socketcan)

add_executable(thermostat thermostat.cpp)
target_compile_features(thermostat PRIVATE cxx_std_20)
target_link_libraries(thermostat PRIVATE
  libcanard
  o1heap
  uavcan
  nunavut
  socketcan
)
