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

    cam1 = camList.GetByIndex(0);

    // qDebug() << "Camera Id" << QString::fromStdString(cam1->DeviceID().c_str());

    cam1->Init();
    cam1->BeginAcquisition();
    timer->start(5);
}

void CameraController::stopCamera() {

  if (cam1 && cam1->IsStreaming()) cam1->EndAcquisition();
  if (cam1 && cam1->IsInitialized()) cam1->DeInit();

  cam1 = nullptr;
  camList.Clear();
  system->ReleaseInstance();
}

void CameraController::acquireFrame() {
  Spinnaker::ImageProcessor processor;
  Spinnaker::ImagePtr cam1frame = cam1->GetNextImage(1000);

  if (cam1frame->IsIncomplete()) {
    qDebug() << "Image Incomplete: " << Spinnaker::Image::GetImageStatusDescription(cam1frame->GetImageStatus());
  } else {
    Spinnaker::ImagePtr convertedImage = processor.Convert(cam1frame, Spinnaker::PixelFormat_RGB8);
    
    cv::Mat frame(
        convertedImage->GetHeight(),
        convertedImage->GetWidth(),
        CV_8UC3,
        convertedImage->GetData()
    );

    cam1frame->Release();
    QImage qimg(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    emit newFrame(qimg.copy());
  }
}
