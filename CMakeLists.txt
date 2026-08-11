cmake_minimum_required(VERSION 3.5)
project( BeerPongSentinelGUI )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SPINNAKER_ROOT "" CACHE PATH "Path to Spinnaker SDK installation (optional)")

find_path(SPINNAKER_INCLUDE_DIR
    NAMES Spinnaker.h
    PATHS ${SPINNAKER_ROOT}/include
)
find_library(SPINNAKER_LIB
    NAMES Spinnaker
    PATHS ${SPINNAKER_ROOT}/lib
)
find_library(SPINNAKER_GENAPI_LIB
    NAMES GenApi_clang140_v3_0
    PATHS ${SPINNAKER_ROOT}/lib
)
find_library(SPINNAKER_GCBASE_LIB
    NAMES GCBase_clang140_v3_0
    PATHS ${SPINNAKER_ROOT}/lib
)

if(NOT SPINNAKER_INCLUDE_DIR OR NOT SPINNAKER_LIB OR NOT SPINNAKER_GENAPI_LIB OR NOT SPINNAKER_GCBASE_LIB)
    message(FATAL_ERROR "Spinnaker SDK not found. Set -DSPINNAKER_ROOT=/path/to/spinnaker")
endif()
find_package( OpenCV REQUIRED )
message(STATUS "OpenCV_VERSION: ${OpenCV_VERSION}")
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Enable Qt auto tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


add_executable(BeerPongSentinelGUI
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    CameraController.cpp
    CameraController.h
    ImageProcessing.cpp
    ImageProcessing.h
)

target_include_directories(BeerPongSentinelGUI PRIVATE ${SPINNAKER_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(
  BeerPongSentinelGUI
  ${OpenCV_LIBS}
  Qt6::Widgets
  ${SPINNAKER_LIB}
  ${SPINNAKER_GENAPI_LIB}
  ${SPINNAKER_GCBASE_LIB}
)
