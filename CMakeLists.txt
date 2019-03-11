cmake_minimum_required(VERSION 2.8)

project(smart_pointer)
set(CMAKE_VERBOSE_MAKEFILE ON)
#include_directories("/usr/include/cairo/")
# With xcb

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
CONAN_BASIC_SETUP(TARGETS)
add_executable(${PROJECT_NAME} "main.cpp")
#target_link_libraries(${PROJECT_NAME} xcb cairo)

#interface to lib atomic
add_library (XCB INTERFACE)
set_target_properties(XCB PROPERTIES INTERFACE_LINK_LIBRARIES
  "xcb")


target_link_libraries(${PROJECT_NAME}
  ${CONAN_TARGETS}
  pthread
  #XCB
  #xcb-shm
  #xcb-render
  )

# With xlib
#add_executable(${PROJECT_NAME} "main_xlib.cpp")
#target_link_libraries(${PROJECT_NAME} X11 Xfixes Xcomposite cairo)
