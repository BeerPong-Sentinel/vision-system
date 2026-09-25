#include "../../core/image_processing.h"
#include <opencv2/opencv.hpp>

int main() {
  ImageProcessor processor{"../../core/stereo_data.npz"};
  // Mirror of Google Colab Script 
  
  cv::Point2f im1_1(55.0, 310.0);
  cv::Point2f im1_2(418.0, 323.0);
  cv::Point2f im2_1(335.0, 300.0);
  cv::Point2f im2_2(698.0, 320.0);

  cv::Point3f res1 = processor.getBall3DCoords(im1_1, im2_1);
  cv::Point3f res2 = processor.getBall3DCoords(im1_2, im2_2);
  
  std::cout << "Distance Between Two Points: " << cv::norm(res1 - res2) << std::endl;
  return 0;
}
