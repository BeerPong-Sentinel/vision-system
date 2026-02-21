#pragma once

// 1. Include the specific file that defines the platform macros first
#include <SpinnakerPlatform.h>

// 2. Fix the broken macro. 
// We redefine it to ignore the 'deprecated' attribute and just provide the 'class' keyword.
#undef SPINNAKER_DEPRECATED_CLASS
#define SPINNAKER_DEPRECATED_CLASS(msg) SPINNAKER_API class

// 3. Now include the main library
#include "Spinnaker.h"
#include <SpinGenApi/SpinnakerGenApi.h>

#include <QtCore/QThread>
#include <QTimer>
#include <QtGui/QImage>
#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>

class CameraController : public QObject {
  Q_OBJECT 
public:
  CameraController(QObject* parent = nullptr);
  ~CameraController();

  void startCamera();
  void stopCamera();

signals:
    void newFrame(const QImage& frame);
  
private slots:
  void acquireFrame();

private:
  Spinnaker::SystemPtr system;
  Spinnaker::CameraList camList;
  Spinnaker::CameraPtr cam1;
  QTimer* timer;
};
