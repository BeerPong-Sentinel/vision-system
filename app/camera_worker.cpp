#include "camera_worker.h"
#include <array>

CameraWorker::CameraWorker(QObject *parent, CameraParams params)
    : QObject(parent),
      cams_(std::make_unique<CameraController>()),
      timer(new QTimer(this)) {
  connect(timer, &QTimer::timeout, this, &CameraWorker::grabFrame);
}

CameraWorker::~CameraWorker() {
  if (cams_) cams_->stopCameras();
}

void CameraWorker::start(ConfigData config) {
  cams_->startCameras(config);
  timer->start(1);
}

void CameraWorker::stop() {
  cams_->stopCameras();
}

void CameraWorker::grabFrame() {
  std::array<Frame, 2> frames = cams_->grabFrame();
  emit newFrame(frames.at(0), frames.at(1));
}



