#include "camera_worker.h"
#include <array>

CameraWorker::CameraWorker(QObject *parent) : QObject(parent), timer(new QTimer(this)) {
  connect(timer, &QTimer::timeout, this, &CameraWorker::grabFrame);
  cams_ = std::make_unique<CameraController>();
}

CameraWorker::~CameraWorker() { cams_->stopCameras(); }

void CameraWorker::start() {
  cams_->startCameras();
  timer->start(1);
}

void CameraWorker::stop() {
  cams_->stopCameras();
}

void CameraWorker::grabFrame() {
  std::array<Frame, 2> frames = cams_->grabFrame();
  emit newFrame(frames.at(0), frames.at(1));
}



