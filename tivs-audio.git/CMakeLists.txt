########################################################################
#
# Project-wide settings

# Name of the project.
#
# CMake files in this project can refer to the root source directory
# as ${gtest_SOURCE_DIR} and to the root binary directory as
# ${gtest_BINARY_DIR}.
# Language "C" is required for find_package(Threads).
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.4)
PROJECT(xbot CXX C)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/cmake)
SET(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include (Dependence)
include (MakeFlags)
include (Functions)

SET(COMMON_PROJECT_DEPENDENCIES boost_date_time cetty tivs mongoclient)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)

ADD_SUBDIRECTORY(src)
