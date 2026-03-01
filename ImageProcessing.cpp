#include "ImageProcessing.h"

#include "CameraParams.h"
#include <QtCore/QDebug>

using namespace cv;

ImageProcessor::ImageProcessor(QObject* parent) : QObject(parent) {
}

ImageProcessor::~ImageProcessor() {
}

void ImageProcessor::processFrames(const Frame& frame1, const Frame& frame2) {
  // Copy Frames to Modify
  Frame processed1;
  Frame processed2;

  processed1.raw = frame1.raw.clone();
  processed2.raw = frame2.raw.clone();

  Mat thresh1, thresh2;

  // processed1.thresh = binaryThreshold(processed1.raw);
  // processed2.thresh = binaryThreshold(processed2.raw);
  
  processed1.thresh = hsvThreshold(processed1.raw);
  processed2.thresh = hsvThreshold(processed2.raw);

  emit newProcessedFrame(processed1, processed2);
}

cv::Mat ImageProcessor::binaryThreshold(cv::Mat raw) {
  int thresholdVal = 127;
  int max_binary_value = 255;
  int threshold_type = 0; // Thresh Binary
                           
  Mat gray, thresh;
  cvtColor(raw, gray, COLOR_BGR2GRAY);
  threshold(gray, thresh, thresholdVal, max_binary_value, threshold_type);
 
  return thresh;
}

cv::Mat ImageProcessor::hsvThreshold(cv::Mat raw) {
  
  Mat hsv_image;
  cvtColor(raw, hsv_image, COLOR_BGR2HSV);

  // Scalar lower_bound = Scalar(m_hueMin, m_saturationMin, m_valueMin);
  // Scalar upper_bound = Scalar(m_hueMax, m_saturationMax, m_valueMax);
  
  Scalar lower_bound = Scalar(20, 40 , 10);
  Scalar upper_bound = Scalar(100, 255, 255);
  
  Mat masked;
  inRange(hsv_image, lower_bound, upper_bound, masked);

  return masked;
}

void ImageProcessor::updateHSVParams(const int h_min, const int h_max, const int s_min, const int s_max, const int v_min, const int v_max) {
   m_hueMin = h_min;
   m_hueMax = h_max;
   m_saturationMin = s_min;
   m_saturationMax = s_max;
   m_valueMin = v_min;
   m_valueMax = v_max;
 }
