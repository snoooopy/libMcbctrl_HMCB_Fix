cmake_minimum_required(VERSION 2.8.3)
cmake_policy(SET CMP0048 NEW)

if(NOT DEFINED OSS_VERSION)
    set(OSS_VERSION "1.0.0.0")
endif()

project(mcbctrl VERSION ${OSS_VERSION})

string(TIMESTAMP NUCTECH_VERSION_BUILD_TIME "%Y-%m-%d %H:%M:%S")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../include/VersionConfig.h.in ${CMAKE_CURRENT_SOURCE_DIR}/../include/VersionConfig.h)


if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()


find_package(Boost REQUIRED)
#find_package(Qt5 COMPONENTS Core Network  REQUIRED)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-D_DEBUG)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    add_definitions(-D_RELEASE)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")

if(UNIX)
    add_definitions(-DUNIX)
endif(UNIX)

add_definitions(-DQT)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
include_directories (
        ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/../src)

#include_directories(${catkin_INCLUDE_DIRS})
#link_directories(${catkin_LIBRARY_DIRS})
#link_directories(./lib)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



set(SRC_CPP_FILE MCBCtrl.cpp
    CB_bsp.cpp
    ECS_func.cpp
    #ECS_funcnew.cpp
    ETH_func.cpp
    FPGA_cfg.cpp
    IniFile.cpp
    MCBCtrlC.cpp
    MCBCtrlC_HMCB4.cpp
    MCBThread.cpp
    RequestSocket.cpp
    #StdAfx.cpp
    nuctool.cpp
    logAdapter.cpp

)
set(SRC_INCLUDE_FILE MCBCtrl.h
    CB_bsp.h
    Config.h
    ECS_func.h
    ETH_func.h
    FPGA_cfg.h
    including.h
    IniFile.h
    MCBCtrlC.h
    MCB_param.h
    MCBThread.h
    memory_map.h
    replenish_type.hpp
    RequestSocket.h
    Resource.h
    SocketBuffer.h
    StdAfx.h
    type_def.h
    nuctool.h
    logAdapter.h
)

add_library(mcbctrl SHARED
${SRC_INCLUDE_FILE}
${SRC_CPP_FILE}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    )

target_link_libraries(mcbctrl pthread boost_thread )

SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../Bin/${CMAKE_BUILD_TYPE})
#SET(LIBRARY_OUTPUT_PATH /home/nuctech/tmp/HMCB)

include(CMakePrintHelpers)
