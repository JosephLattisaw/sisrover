cmake_minimum_required(VERSION 3.0.0)
project(camsrv VERSION 4.0.0)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

find_package(Boost 1.74.0 COMPONENTS program_options)

add_definitions(-std=c++17)

add_executable(camsrv main.cpp)
target_link_libraries(camsrv ${Boost_LIBRARIES})

