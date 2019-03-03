cmake_minimum_required(VERSION 2.8)

project(smart_pointer)
set(CMAKE_VERBOSE_MAKEFILE ON)
include_directories("/usr/include/cairo/")
# With xcb
add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} xcb cairo)

# With xlib
#add_executable(${PROJECT_NAME} "main_xlib.cpp")
#target_link_libraries(${PROJECT_NAME} X11 Xfixes Xcomposite cairo)
