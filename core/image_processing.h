#pragma once

#include "../camera_params.h"
#include <opencv2/opencv.hpp>
#include <string>

class ImageProcessor {
public:
  ImageProcessor(std::string stereo_data_path);
  ~ImageProcessor();

  cv::Mat greenThreshold(const cv::Mat& raw);
  cv::Mat blueThreshold(const cv::Mat& raw);
  cv::Mat tennisThreshold(const cv::Mat& raw);
  cv::Mat hsvThreshold(const cv::Mat& raw);
  cv::Point2f detectBall(const cv::Mat& raw, const cv::Mat& thresh, bool &hasBall, cv::Point2f &center, float &radius, cv::Mat &annotated);

  cv::Point3f getBall3DCoords(cv::Point2f pos_1, cv::Point2f pos_2);

  void updateHSVParams(const int h_min, const int h_max, const int s_min, const int s_max, const int v_min, const int v_max);
  void updateThreshParams(int margin, int lowThresh, int minSize, int margin_2);
  
private:
  int m_hueMin, m_hueMax;
  int m_saturationMin, m_saturationMax;
  int m_valueMin, m_valueMax;
  int m_greenMargin = 5;
  int m_blueMargin = 50;
  int m_lowThresh = 10;
  int m_minSize = 5;

  cv::Matx34d m_projMat1 = cv::Matx34d::zeros();
  cv::Matx34d m_projMat2 = cv::Matx34d::zeros();
  
  cv::Matx33d m_K_1 = cv::Matx33d::zeros();
  cv::Matx33d m_K_2 = cv::Matx33d::zeros();
  
  cv::Matx33d m_R = cv::Matx33d::zeros();
  cv::Matx31d m_T = cv::Matx31d::zeros();

  cv::Mat m_D_1;
  cv::Mat m_D_2;
};
