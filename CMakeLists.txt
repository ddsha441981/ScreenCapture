cmake_minimum_required(VERSION 3.10)
project(ScreenCapture)

set(CMAKE_CXX_STANDARD 17)

include_directories(
        ${CMAKE_SOURCE_DIR}
)

set(SOURCE_FILES
        rectangle_capture.cpp
        stb_image_write_impl.cpp
)

add_executable(area_capture_tool ${SOURCE_FILES})

find_package(X11 REQUIRED)

target_link_libraries(area_capture_tool
        ${X11_LIBRARIES}
        Xfixes
)

