#include <QApplication>
#include "app/MainWindow.h"
#include <iostream>
#include "yaml-cpp/yaml.h"

int main(int argc, char *argv[])
{
    std::cout << "Starting Application" << std::endl;
    auto sys = Spinnaker::System::GetInstance();
    auto v = sys->GetLibraryVersion();
    std::cout << "Spinnaker " << v.major << "." << v.minor << "." << v.type << "." << v.build << std::endl;
    sys->ReleaseInstance();
    QApplication app(argc, argv);

    ConfigData config;
    // Unpack Config File
    YAML::Node config_file = YAML::LoadFile("config/config.yaml");

    config.hsv.h_min = config_file["thresholding"]["hsv"]["hue_min"].as<int>();
    config.hsv.h_max = config_file["thresholding"]["hsv"]["hue_max"].as<int>();
    config.hsv.s_min = config_file["thresholding"]["hsv"]["val_min"].as<int>();
    config.hsv.s_max = config_file["thresholding"]["hsv"]["val_max"].as<int>();
    config.hsv.v_min = config_file["thresholding"]["hsv"]["sat_min"].as<int>();
    config.hsv.v_max = config_file["thresholding"]["hsv"]["sat_max"].as<int>();

    config.cam.fps = config_file["cam"]["fps"].as<double>();
    config.cam.exposure_time = config_file["cam"]["exposure_time"].as<double>();
    MainWindow w(nullptr, config);
    w.show();
    return app.exec();
}
