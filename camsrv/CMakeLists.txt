cmake_minimum_required(VERSION 3.0.0)
project(camsrv VERSION 4.0.0)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

function(add_include_dir parent_dir inc_usage)
    unset(PARENT_DIR CACHE)
    find_path(PARENT_DIR NAMES ${parent_dir})
    if(NOT PARENT_DIR) 
        message(FATAL_ERROR "could not find ${parent_dir} include directories")
    else()
        message(STATUS "found ${parent_dir} directories => ${PARENT_DIR}/${parent_dir}")
    endif() 
    
    if(inc_usage) 
        include_directories(${PARENT_DIR}/${parent_dir})
    endif()
endfunction()

function(find_required_libs lib)
    unset(FCN_LIB CACHE)
    find_library(FCN_LIB ${lib})
    if(NOT FCN_LIB)
        message(FATAL_ERROR "could not find ${lib} libraries")
    else()
        message(STATUS "found ${lib} library => ${FCN_LIB}")
        find_path(FCN_LIB_PATH NAMES lib/lib${lib}.so)
        link_directories(${FCN_LIB_PATH}/lib)
        unset(FCN_LIB_PATH CACHE)
    endif()
endfunction()

add_include_dir(liveMedia true) #liveMedia include directories
add_include_dir(groupsock true) #groupsock include directories
add_include_dir(UsageEnvironment true) #UsageEnvironment include directories
add_include_dir(BasicUsageEnvironment true) #BasicUsageEnvironment include directories
add_include_dir(linux/videodev2.h false)
add_include_dir(libv4l2.h false)

find_required_libs(liveMedia)
find_required_libs(groupsock)
find_required_libs(UsageEnvironment)
find_required_libs(v4l2)

find_package(Boost 1.74.0 REQUIRED COMPONENTS program_options)

add_definitions(-std=c++17)

add_executable(camsrv main.cpp controller.cpp server.cpp ipcamera.cpp webcamera.cpp camera.cpp)
target_link_libraries(camsrv ${Boost_LIBRARIES} pthread v4l2 liveMedia groupsock UsageEnvironment BasicUsageEnvironment)

