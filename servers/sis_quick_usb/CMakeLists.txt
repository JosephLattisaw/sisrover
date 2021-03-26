cmake_minimum_required(VERSION 3.0.0)
project(sis_quick_usb VERSION 0.1.0)

include(CTest)
enable_testing()

add_definitions(-std=c++17)

include_directories(include/sis_quick_usb)

add_library(sis_quick_usb SHARED sis_quick_usb.cpp)
target_link_libraries(sis_quick_usb quickusb usb)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
