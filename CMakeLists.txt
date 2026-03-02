cmake_minimum_required(VERSION 3.5)
project( BeerPongSentinelGUI )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Adjust path to your installation
set(SPINNAKER_ROOT "/Applications/Spinnaker/")

find_path(SPINNAKER_INCLUDE_DIR
    NAMES Spinnaker.h
    PATHS ${SPINNAKER_ROOT}/include
    NO_DEFAULT_PATH
)

set(SPINNAKER_LIB "I:/SoftwareLibraries/SpinnakerSDK/lib64/vs2015/Spinnaker_v140.lib")

set(SPINNAKER_ROOT "I:/SoftwareLibraries/SpinnakerSDK")

if(NOT SPINNAKER_INCLUDE_DIR OR NOT SPINNAKER_LIB)
    message(FATAL_EROR "Spinnaker not found")
endif()

set(OpenCV_DIR "I:/SoftwareLibraries/opencv/build")
set(CMAKE_PREFIX_PATH "I:/SoftwareLibraries/Qt/6.10.2/msvc2022_64")

find_package( OpenCV REQUIRED )
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
)
