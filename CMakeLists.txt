cmake_minimum_required(VERSION 2.8.3)
cmake_policy(SET CMP0048 NEW)
project(mcbctrl VERSION ${OSS_VERSION_mcbctrl})

string(TIMESTAMP NUCTECH_VERSION_BUILD_TIME "%Y-%m-%d %H:%M:%S")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/VersionConfig.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/VersionConfig.h)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

## Compile as C++11, supported in ROS Kinetic and newer
# add_compile_options(-std=c++11)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
)

## System dependencies are found with CMake's conventions
find_package(Boost REQUIRED)

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
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

#include_directories(${catkin_INCLUDE_DIRS})
#link_directories(${catkin_LIBRARY_DIRS})
#link_directories(./lib)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



set(SRC_CPP_FILE 
    src/MCBCtrl.cpp
    src/CB_bsp.cpp
    src/ECS_func.cpp
    #ECS_funcnew.cpp
    src/ETH_func.cpp
    src/FPGA_cfg.cpp
    src/IniFile.cpp
    src/MCBCtrlC.cpp
    src/MCBCtrlC_HMCB4.cpp
    src/MCBThread.cpp
    src/RequestSocket.cpp
    #StdAfx.cpp
    src/nuctool.cpp
    src/logAdapter.cpp

)
set(SRC_INCLUDE_FILE 
    src/MCBCtrl.h
    src/CB_bsp.h
    src/Config.h
    src/ECS_func.h
    src/ETH_func.h
    src/FPGA_cfg.h
    src/including.h
    src/IniFile.h
    src/MCBCtrlC.h
    src/MCB_param.h
    src/MCBThread.h
    src/memory_map.h
    src/replenish_type.hpp
    src/RequestSocket.h
    src/Resource.h
    src/SocketBuffer.h
    src/StdAfx.h
    src/type_def.h
    src/nuctool.h
    src/logAdapter.h
)

add_library(mcbctrl SHARED
${SRC_INCLUDE_FILE}
${SRC_CPP_FILE}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    )

target_link_libraries(mcbctrl pthread boost_thread )

# SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../Bin/${CMAKE_BUILD_TYPE})
#SET(LIBRARY_OUTPUT_PATH /home/nuctech/tmp/HMCB)

include(CMakePrintHelpers)