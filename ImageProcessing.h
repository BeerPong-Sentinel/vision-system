#pragma once

#include "CameraParams.h"
#include <QtCore/QThread>
#include <opencv2/opencv.hpp>

class ImageProcessor : public QObject
{
  Q_OBJECT
public:
  ImageProcessor(QObject *parent = nullptr);
  ~ImageProcessor();

signals:
  void newProcessedFrame(const Frame &frame1, const Frame &frame2, const Frame &frame3);

public slots:
  void processFrames(const Frame &frame1, const Frame &frame2, const Frame &frame3);
  void updateHSVParams(const int h_min, const int h_max, const int s_min, const int s_max, const int v_min, const int v_max);

private:
  int m_hueMin, m_hueMax;
  int m_saturationMin, m_saturationMax;
  int m_valueMin, m_valueMax;

  cv::Mat greenThreshold(cv::Mat raw);
  cv::Mat hsvThreshold(cv::Mat raw);
  cv::Point2f detectBall(cv::Mat raw, cv::Mat thresh, bool &hasBall, cv::Point2f &center, float &radius, cv::Mat &annotated);
};
