#include "camera_worker.h"
#include <array>

CameraWorker::CameraWorker(QObject *parent, CameraParams params)
    : QObject(parent),
      cams_(std::make_unique<CameraController>(params)),
      timer(new QTimer(this)),
      m_params(params){
  connect(timer, &QTimer::timeout, this, &CameraWorker::grabFrame);
}

CameraWorker::~CameraWorker() {
}

void CameraWorker::start() {
  cams_->startCameras();
  timer->start(1);
}

void CameraWorker::stop() {
  std::cout << "Stopping Camera Capture" << std::endl;
  cams_->stopCameras();
}

void CameraWorker::grabFrame() {
  std::array<Frame, 2> frames = cams_->grabFrame();
  emit newFrame(frames.at(0), frames.at(1));
}



