add_library(socketcan STATIC socketcan.cpp)
target_include_directories(socketcan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(socketcan PUBLIC libcanard)
