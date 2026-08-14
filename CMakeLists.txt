cmake_minimum_required(VERSION 3.16)
project(BeerPongSentinelGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt needs these on for any target that uses Q_OBJECT/signals/slots/.ui files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(core)
add_subdirectory(app)

option(BUILD_SANDBOX "Build scratch/sandbox executables" OFF)
if(BUILD_SANDBOX)
    add_subdirectory(scratch)
endif()
