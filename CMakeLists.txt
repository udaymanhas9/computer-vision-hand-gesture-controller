cmake_minimum_required(VERSION 3.10)
project(HandGestureController)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(hand_gesture main.cpp)

target_link_libraries(hand_gesture
    ${OpenCV_LIBS}
    "-framework ApplicationServices"
)
