cmake_minimum_required(VERSION 3.16)
project(Mine++Server)

set(CMAKE_CXX_STANDARD 20)

INCLUDE_DIRECTORIES(libs/asio-1.18.2/include
        ../MineNet)

add_executable(Mine++Server main.cpp)
if (WIN32)
    target_link_libraries(Mine++Server -lwsock32 ws2_32.lib)
else()
    target_link_libraries(Mine++Server -lpthread)
endif()