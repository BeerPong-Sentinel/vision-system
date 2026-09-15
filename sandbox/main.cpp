#include "../core/camera_controller.h"
#include <array>
#include <opencv2/opencv.hpp>
#include <format>
#include <string>

int main() {
  CameraController* controller = new CameraController();

  controller->startCameras();
  constexpr int numImages = 10;
  for (int i = 0; i < numImages; i++) {
    std::stringstream ss;
    ss << "frame" << i << ".png";
    std::array<Frame, 2> grame = controller->grabFrame(); 
    std::cout <<"Press Enter to take picture: " << i << std::endl;
    std::cin.get();

    cv::imwrite(ss.str(), grame.at(1).raw);
  }

  return 0;
}
