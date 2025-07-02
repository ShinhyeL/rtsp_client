cmake_minimum_required(VERSION 3.10)
project(RTSPClient)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(rtsp_client main.cpp)
target_link_libraries(rtsp_client ${OpenCV_LIBS} rt)
