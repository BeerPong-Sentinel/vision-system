#pragma once

#include "../camera_params.h"
#include "../core/image_processing.h"
#include <QtCore/QThread>

class ImageProcessorWorker :public QObject
{
  Q_OBJECT

public:
  ImageProcessorWorker(QObject *parent = nullptr);
  ~ImageProcessorWorker() {};

signals:
  void newProcessedFrame(const Frame &frame1, const Frame &frame2);

public slots:
  void processFrames(const Frame &frame1, const Frame &frame2);
  void updateHSVParams(const int h_min, const int h_max, const int s_min, const int s_max, const int v_min, const int v_max);
  void updateThreshParams(int margin, int lowThresh, int minSize, int margin_2);

private:
  ImageProcessor processor;
};
