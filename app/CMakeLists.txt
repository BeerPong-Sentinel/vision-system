find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(balltracker_app
    main.cpp
    MainWindow.cpp
    MainWindow.h
    mainwindow.ui
    camera_worker.cpp
    camera_worker.h
    image_processing_worker.cpp
    image_processing_worker.h
)

target_link_libraries(balltracker_app PRIVATE
    balltracker_core
    Qt6::Widgets
)

target_include_directories(balltracker_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
