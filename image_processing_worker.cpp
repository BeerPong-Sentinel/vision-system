#include "image_processing_worker.h"
#include <QtCore/QDebug>

ImageProcessorWorker::ImageProcessorWorker(QObject *parent) : QObject(parent)
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

  emit newProcessedFrame(processed1, processed2);
}

void ImageProcessorWorker::updateHSVParams(const int h_min, const int h_max, const int s_min, const int s_max, const int v_min, const int v_max) {
  processor.updateHSVParams(h_min, h_max, s_min, s_max, v_min, v_max);
  qDebug() << "Updated HSV Params";
}

void ImageProcessorWorker::updateThreshParams(int margin, int lowThresh, int minSize, int margin_2) {
  qDebug() << "Updated Tennis Thresh Params";
  processor.updateThreshParams(margin, lowThresh, minSize, margin_2);
}
