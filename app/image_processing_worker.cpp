#include "image_processing_worker.h"
#include <QtCore/QDebug>

ImageProcessorWorker::ImageProcessorWorker(QObject *parent, HSVThreshParams hsv_params) : 
  QObject(parent),
  processor("core/stereo_data.npz", hsv_params)
{

}

void ImageProcessorWorker::processFrames(const Frame &frame1, const Frame &frame2) {
  Frame processed1;
  Frame processed2;

  processed1.raw = frame1.raw.clone();
  processed2.raw = frame2.raw.clone();

  processed1.thresh = processor.hsvThreshold(processed1.raw);
  processed2.thresh = processor.hsvThreshold(processed2.raw);

  processor.detectBall(processed1.raw, processed1.thresh, processed1.hasBall, processed1.ballCenter, processed1.ballRadius, processed1.annotated);
  processor.detectBall(processed2.raw, processed2.thresh, processed2.hasBall, processed2.ballCenter, processed2.ballRadius, processed2.annotated);

  processed1.cam_ts = frame1.cam_ts;
  processed1.program_ts = frame1.program_ts;

  processed2.cam_ts = frame2.cam_ts;
  processed2.program_ts = frame2.program_ts;

  emit newProcessedFrame(processed1, processed2);

  // Space Transformation
  if (!processed1.hasBall || !processed2.hasBall) {
    // emit newBallPosition(std::nullopt);
  } else {
    emit newBallPosition(processor.getBall3DCoords(processed1.ballCenter, processed2.ballCenter));
  }
}

void ImageProcessorWorker::updateHSVParams(HSVThreshParams params) {
  processor.updateHSVParams(params);
  qDebug() << "Updated HSV Params";
}

void ImageProcessorWorker::updateThreshParams(int margin, int lowThresh, int minSize, int margin_2) {
  qDebug() << "Updated Tennis Thresh Params";
  processor.updateThreshParams(margin, lowThresh, minSize, margin_2);
}
