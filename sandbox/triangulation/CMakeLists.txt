cmake_minimum_required(VERSION 3.5)
project( TriangulationTest )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "../../lib/Eigen")

find_package( OpenCV REQUIRED )
find_package(Eigen3 REQUIRED)

find_library(CNPY_LIBRARY NAMES cnpy REQUIRED)
find_path(CNPY_INCLUDE_DIR NAMES cnpy.h REQUIRED)

message(STATUS "OpenCV_VERSION: ${OpenCV_VERSION}")
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")

add_executable(TriangulationTest
  main.cpp
  ../../core/image_processing.h
  ../../core/image_processing.cpp
)

target_include_directories(
  TriangulationTest PRIVATE
  ${OpenCV_INCLUDE_DIRS}
  ${CNPY_INCLUDE_DIR}
)

target_link_libraries(
  TriangulationTest PRIVATE
  ${OpenCV_LIBS}
  ${CNPY_LIBRARY}
  Eigen3::Eigen
  z
)

