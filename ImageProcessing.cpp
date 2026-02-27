#include "ImageProcessing.h"

#include "CameraParams.h"
#include <QtCore/QDebug>
#include <opencv2/opencv.hpp>

using namespace cv;

ImageProcessor::ImageProcessor(QObject* parent) : QObject(parent) {

}

ImageProcessor::~ImageProcessor() {

}

void ImageProcessor::processFrames(const Frame& frame1, const Frame& frame2) {
  // Copy Frames to Modify
  Frame processed1 = frame1;
  Frame processed2 = frame2;

  int thresholdVal = 127;
  int max_binary_value = 255;
  int threshold_type = 0; // Thresh Binary

  Mat gray1, thresh1;
  Mat gray2, thresh2;

  cvtColor(frame1.raw, gray1, COLOR_BGR2GRAY);
  cvtColor(frame2.raw, gray2, COLOR_BGR2GRAY);

  threshold(gray1, thresh1, thresholdVal, max_binary_value, threshold_type);
  threshold(gray2, thresh2, thresholdVal, max_binary_value, threshold_type);

  processed1.thresh = thresh1;
  processed2.thresh = thresh2;

  emit newProcessedFrame(processed1, processed2);
}
