cmake_minimum_required(VERSION 4.3)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(TPC_Control LANGUAGES CXX)

add_subdirectory(utilities)
add_subdirectory(tpc_core)
add_subdirectory(tpc_analytics)
add_subdirectory(tpc_system)
