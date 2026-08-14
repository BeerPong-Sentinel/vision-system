if(NOT DEFINED SPINNAKER_ROOT)
    message(FATAL_ERROR "SPINNAKER_ROOT must be set, e.g. -DSPINNAKER_ROOT=/opt/spinnaker")
endif()

find_library(SPINNAKER_LIB
    NAMES Spinnaker
    PATHS "${SPINNAKER_ROOT}/lib"
)

find_path(SPINNAKER_INCLUDE_DIR
    NAMES Spinnaker.h
    PATHS "${SPINNAKER_ROOT}/include"
)

find_package(OpenCV REQUIRED)

add_library(balltracker_core STATIC
    camera_controller.cpp
    image_processing.cpp
)

target_include_directories(balltracker_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SPINNAKER_INCLUDE_DIR}
)

target_link_libraries(balltracker_core PUBLIC
    ${OpenCV_LIBS}
    ${SPINNAKER_LIB}
)
