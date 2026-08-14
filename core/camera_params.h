#pragma once

#include <opencv2/opencv.hpp>
#include <chrono>
#include <string>

// Frame object holds all relevant data for each image.
// As it passes through the pipeline, it will be filled out
// Should start as just raw, and t

struct Frame
{
  cv::Mat raw;
  cv::Mat thresh;
  cv::Mat annotated;
  std::chrono::steady_clock::time_point t;
  cv::Point2f ballCenter;
  float ballRadius = 0;
  bool hasBall = false;
};

struct CameraDetails {
  std::string name;
  std::string deviceID;
};
