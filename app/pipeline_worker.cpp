#include "pipeline_worker.h"
#include <opencv2/opencv.hpp>

void PipelineWorker::startCapture() {
  m_cams.startCameras();
  long i = 0;
  constexpr int GUI_REFRESH_WAIT = 20;
  int64_t prev_ts = 0;

  while (!QThread::currentThread()->isInterruptionRequested()) {
    std::array<Frame, 2> frames = m_cams.grabFrame();
    // Thresholding
    std::array<Frame, 2> processed_frames;
    
    for(int i = 0; i < 2; i++) {
      Frame& processed_frame = processed_frames[i];
      processed_frame.raw = frames[i].raw.clone();
      processed_frame.thresh = m_proc.hsvThreshold(processed_frame.raw);

      m_proc.detectBall(
        processed_frame.raw,
        processed_frame.thresh,
        processed_frame.hasBall,
        processed_frame.ballCenter,
        processed_frame.ballRadius,
        processed_frame.annotated
      );

      processed_frame.cam_ts = frames[i].cam_ts;
      processed_frame.program_ts = frames[i].program_ts;
    }
    
    if (i % GUI_REFRESH_WAIT == 0) {
      std::cout << "FPS: " << static_cast<double>(std::pow(10, 9)) / (processed_frames[0].cam_ts - prev_ts) << "\n";
      emit newFrame(processed_frames[0], processed_frames[1]);
    }
    
    prev_ts = processed_frames[0].cam_ts;
    i++;
  }

  // Cleanup Resources
  m_cams.stopCameras();
  emit finished();
}

void PipelineWorker::stopCapture() {

}
