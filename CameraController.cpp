#include "CameraController.h"
#include <QtCore/QDebug>
#include <opencv2/opencv.hpp>

CameraController::CameraController(QObject* parent) : QObject(parent), timer(new QTimer(this)) {
  connect(timer, &QTimer::timeout, this, &CameraController::acquireFrame);
}

CameraController::~CameraController() {
  stopCamera();
}

void CameraController::startCamera() {
    // Retrieve singleton reference to system object
    system = Spinnaker::System::GetInstance();

    // Retrieve list of cameras from the system
    camList = system->GetCameras();

    const unsigned int numCameras = camList.GetSize();

    qDebug() << "Number of Cameras Detected: " << numCameras;

    if (numCameras == 0)
    {
        qCritical() << "Not enough cameras!";
        return;
    }
    
    cam1 = camList.GetBySerial(CAM1DEVICEID);
    cam2 = camList.GetBySerial(CAM2DEVICEID);

    if (cam1) {
      cam1->Init();
      cam1->BeginAcquisition();
    } else {
      qDebug() << "Could Not Connect to Camera 1";
    }

    if (cam2) {
      cam2->Init();
      cam2->BeginAcquisition();
    } else {
      qDebug() << "Could Not Connect to Camera 2";
    }

    // qDebug() << "Camera Id" << QString::fromStdString(cam1->DeviceID().c_str());
    timer->start(5);
}

void CameraController::stopCamera() {

  if (cam1 && cam1->IsStreaming()) cam1->EndAcquisition();
  if (cam1 && cam1->IsInitialized()) cam1->DeInit();

  if (cam2 && cam2->IsStreaming()) cam2->EndAcquisition();
  if (cam2 && cam2->IsInitialized()) cam2->DeInit();

  cam1 = nullptr;
  cam2 = nullptr;
  camList.Clear();
  system->ReleaseInstance();
}

void CameraController::acquireFrame() {
  const int width = 720;  
  const int height = 540; 

  // Default black images
  QImage qimg1(width, height, QImage::Format_RGB888);
  QImage qimg2(width, height, QImage::Format_RGB888);
  qimg1.fill(Qt::black);
  qimg2.fill(Qt::black);

  if (cam1) {
    Spinnaker::ImageProcessor processor;
    Spinnaker::ImagePtr cam1frame = cam1->GetNextImage(1000);

    if (cam1frame->IsIncomplete()) {
      qDebug() << "Image1 Incomplete: " << Spinnaker::Image::GetImageStatusDescription(cam1frame->GetImageStatus());
    } else {
      Spinnaker::ImagePtr convertedImage = processor.Convert(cam1frame, Spinnaker::PixelFormat_RGB8);

      int width  = static_cast<int>(convertedImage->GetWidth());
      int height = static_cast<int>(convertedImage->GetHeight());
      int stride = static_cast<int>(convertedImage->GetStride());

      uchar* data = static_cast<uchar*>(convertedImage->GetData());

      QImage tmp(
          data,
          width,
          height,
          stride,
          QImage::Format_RGB888
      );
      qimg1 = tmp.copy(); 
      cam1frame->Release();
    }
  }

  if (cam2) {
    Spinnaker::ImageProcessor processor;
    Spinnaker::ImagePtr cam2frame = cam2->GetNextImage(1000);

    if (cam2frame->IsIncomplete()) {
      qDebug() << "Image2 Incomplete: " << Spinnaker::Image::GetImageStatusDescription(cam2frame->GetImageStatus());
    } else {
      Spinnaker::ImagePtr convertedImage = processor.Convert(cam2frame, Spinnaker::PixelFormat_RGB8);
      int width  = static_cast<int>(convertedImage->GetWidth());
      int height = static_cast<int>(convertedImage->GetHeight());
      int stride = static_cast<int>(convertedImage->GetStride());

      uchar* data = static_cast<uchar*>(convertedImage->GetData());

      QImage tmp(
          data,
          width,
          height,
          stride,
          QImage::Format_RGB888
      );
      qimg2 = tmp.copy(); 
      cam2frame->Release();
    }
  }

  emit newFrame(qimg1.copy(), qimg2.copy());
}
