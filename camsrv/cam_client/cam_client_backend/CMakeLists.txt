cmake_minimum_required(VERSION 3.0.0)
project(cam_client_backend VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(cam_client_backend SHARED cam_client_backend.cpp)
add_executable(cam_client_backend_test cam_client_backend.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
