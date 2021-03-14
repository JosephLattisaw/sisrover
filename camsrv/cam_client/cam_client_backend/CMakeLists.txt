cmake_minimum_required(VERSION 3.0.0)
project(cam_client_backend VERSION 0.1.0)

find_package(Boost 1.74.0 REQUIRED)
add_definitions(-std=c++17)

include(CTest)
enable_testing()

include_directories(../..)

set(cam_client_backend_SRC cam_client_backend.cpp cam_client.cpp)

add_library(cam_client_backend SHARED ${cam_client_backend_SRC})
target_link_libraries(cam_client_backend ${Boost_LIBRARIES} pthread)

add_executable(cam_client_backend_test cam_client_backend.cpp ${cam_client_backend_SRC})
target_link_libraries(cam_client_backend_test ${Boost_LIBRARIES} pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
