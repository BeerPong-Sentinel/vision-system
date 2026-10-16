#pragma once

#include "../datatypes.h"
#include "../core/image_processing.h"
#include <QtCore/QThread>
#include <optional>

class ImageProcessorWorker :public QObject
{
  Q_OBJECT

public:
  ImageProcessorWorker(QObject *parent = nullptr, HSVThreshParams hsv_params = {});
  ~ImageProcessorWorker() {};

signals:
  void newProcessedFrame(const Frame &frame1, const Frame &frame2);
  void newBallPosition(std::optional<cv::Point3f> pos);

public slots:
  void processFrames(const Frame &frame1, const Frame &frame2);
  void updateHSVParams(HSVThreshParams params);
  void updateThreshParams(int margin, int lowThresh, int minSize, int margin_2);

private:
  ImageProcessor processor;
};
