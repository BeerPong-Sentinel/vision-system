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
  void newProcessedFrame(const Frame &frame1, const Frame &frame2);

public slots:
  void processFrames(const Frame &frame1, const Frame &frame2);
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

  cv::Mat greenThreshold(cv::Mat raw);
  cv::Mat blueThreshold(cv::Mat raw);
  cv::Mat tennisThreshold(cv::Mat raw);
  cv::Mat hsvThreshold(cv::Mat raw);
  cv::Point2f detectBall(cv::Mat raw, cv::Mat thresh, bool &hasBall, cv::Point2f &center, float &radius, cv::Mat &annotated);
};
