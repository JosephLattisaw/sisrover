cmake_minimum_required(VERSION 3.0.0)
project(joysrv VERSION 0.1.0)

include(CTest)
enable_testing()

add_definitions(-std=c++17)
find_package(Boost 1.74.0 REQUIRED)

add_executable(joysrv main.cpp joysrv.cpp server.cpp)
target_link_libraries(joysrv pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
