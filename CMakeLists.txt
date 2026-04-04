cmake_minimum_required(VERSION 3.15)

if(NOT SKBUILD)
    message("-- Running with CMake")
else()
    message("-- Running from 'scikit-build-core'")
endif()

project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDART 20)
set(CMAKE_CXX_STANDART_REQUIRED ON)

if (CMAKE_VERSION VERSION_LESS 3.18)
    set(DEV_MODULE Development)
else()
    set(DEV_MODULE Development.Module)
endif()

set(CARTOGRAPHER_MAJOR_VERSION 1)
set(CARTOGRAPHER_MINOR_VERSION 0)
set(CARTOGRAPHER_PATCH_VERSION 0)
set(CARTOGRAPHER_VERSION ${CARTOGRAPHER_MAJOR_VERSION}.${CARTOGRAPHER_MINOR_VERSION}.${CARTOGRAPHER_PATCH_VERSION})
set(CARTOGRAPHER_SOVERSION ${CARTOGRAPHER_MAJOR_VERSION}.${CARTOGRAPHER_MINOR_VERSION})
option(BUILD_GRPC "build Cartographer gRPC support" false)
set(CARTOGRAPHER_HAS_GRPC false)
option(BUILD_PROMETHEUS "build Prometheus monitoring support" false)

include(FindPackageHandleStandardArgs)
include("${PROJECT_SOURCE_DIR}/cmake/functions.cmake")
google_initialize_cartographer_project()

find_package(LuaGoogle REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS iostreams)
find_package(Eigen3 REQUIRED)
find_package(absl REQUIRED)
find_package(Ceres REQUIRED)

# if (NOT TARGET Boost:iostreams)
#     message("creating target boost:iostreams")
#     add_library(Boost::iostreams IMPORTED INTERFACE)
#     set_property(TARGET Boost::iostreams PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIRS})
#     set_property(TARGET Boost::iostreams PROPERTY INTERFACE_LINK_LIBRARIES ${Boost_LIBRARIES})
# endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

find_package(Python COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)
execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_DIR
)
find_package(nanobind CONFIG REQUIRED)

add_subdirectory(carto)
add_subdirectory(src/carto_py_)