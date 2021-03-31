cmake_minimum_required(VERSION 3.0.0)
project(sis_servers)

include(CTest)
enable_testing()

add_subdirectory(camsrv)
add_subdirectory(sis_quick_usb)
add_subdirectory(daqsrv)
