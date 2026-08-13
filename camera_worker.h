#pragma once

#include "camera_params.h"
#include "camera_controller.h"
#include <QtCore/QThread>
#include <QTimer>
#include <QtGui/QImage>

class CameraWorker : public QObject
{
  Q_OBJECT
public:
    CameraWorker(QObject *parent = nullptr);
    ~CameraWorker();

  void start();
  void stop();

signals:
  void newFrame(const Frame &frame1, const Frame &frame2);

private slots:
  void grabFrame();

private:
  std::unique_ptr<CameraController> cams_;
  QTimer *timer;
};
