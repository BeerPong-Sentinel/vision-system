#pragma once

#include "../datatypes.h"
#include "../core/camera_controller.h"
#include <QtCore/QThread>
#include <QTimer>
#include <QtGui/QImage>
#include "../core/image_processing.h"

class PipelineWorker : public QObject
{
  Q_OBJECT
public:
  explicit PipelineWorker(QObject *parent = nullptr, ConfigData params = {}) : 
  QObject(parent),
  m_cams(params.cam),
  m_proc("core/stereo_data.npz", params.hsv) {}

public slots:
  void startCapture();

signals:
  void newFrame(const Frame& frame1, const Frame& frame2);
  void finished();

private:
  void stopCapture();
  ConfigData m_params;
  CameraController m_cams;
  ImageProcessor m_proc;
};
