#pragma once

#include <QtCore/QThread>
#include "CameraParams.h"

class ImageProcessor : public QObject {
  Q_OBJECT
public:
  ImageProcessor(QObject* parent = nullptr);
  ~ImageProcessor();
    
signals:
  void newProcessedFrame(const Frame& frame1, const Frame& frame2);

public slots:
  void processFrames(const Frame& frame1, const Frame& frame2);

};
