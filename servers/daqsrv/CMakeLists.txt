cmake_minimum_required(VERSION 3.0.0)
project(daqsrv VERSION 0.1.0)

include(CTest)
enable_testing()

add_definitions(-std=c++17)
find_package(Boost 1.74.0 REQUIRED COMPONENTS program_options)

add_executable(daqsrv main.cpp daq.cpp server.cpp controller.cpp)
target_link_libraries(daqsrv ${Boost_LIBRARIES} sis_quick_usb pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
