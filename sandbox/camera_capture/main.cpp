#include "../core/camera_controller.h"
#include <array>
#include <opencv2/opencv.hpp>
#include "../lib/cxxopts.hpp"
#include <format>
#include <string>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
  cxxopts::Options options("FrameGrabber", "Grabs Frames for both FLIR Cameras");
  options.add_options()
    ("a,auto", "Take Photos Automatically With 1 sec delay", cxxopts::value<bool>()->default_value("false"))
  ;

  auto result = options.parse(argc, argv);
  bool m_auto = result["auto"].as<bool>();

  CameraController* controller = new CameraController();
  controller->startCameras();

  constexpr int numImages = 20;
  for (int i = 0; i < numImages; i++) {
    std::stringstream ss_1;
    std::stringstream ss_2;
    ss_1 << "images/" << kCam1.name << "_" << i << ".png";
    ss_2 << "images/" << kCam2.name << "_" << i << ".png";

    std::array<Frame, 2> frame = controller->grabFrame(); 
    if (m_auto) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(3s);
      std::cout << "Picture " << i << " has been taken." << std::endl;
    } else {
      std::cout <<"Press Enter to take picture: " << i << std::endl;
      std::cin.get();
    }

    cv::imwrite(ss_1.str(), frame.at(0).raw);
    cv::imwrite(ss_2.str(), frame.at(1).raw);
  }

  return 0;
}
