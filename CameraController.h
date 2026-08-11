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

#include "CameraParams.h"

#include <QtCore/QThread>
#include <QTimer>
#include <QtGui/QImage>
#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>
#include <opencv2/opencv.hpp>
#include <string>

inline const CameraDetails kCam1 = {"Camera 1", "24292737"};
inline const CameraDetails kCam2 = {"Camera 2", "25376121"}; // Badminton Bot Team's Camera Id
// inline const Camera kCam2 = {"Camera 2", "24292752"}; 

class CameraController : public QObject
{
  Q_OBJECT
public:
  CameraController(QObject *parent = nullptr);
  ~CameraController();

  void startCamera();
  void stopCamera();

signals:
  void newFrame(const Frame &frame1, const Frame &frame2);

private slots:
  void acquireFrame();

private:
  Spinnaker::SystemPtr system;
  Spinnaker::CameraList camList;
  Spinnaker::CameraPtr cam1;
  Spinnaker::CameraPtr cam2;
  QTimer *timer;
};
