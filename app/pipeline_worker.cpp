#include "pipeline_worker.h"

#include <opencv2/opencv.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// ============================================
// Start persistent processing threads
// ============================================

void PipelineWorker::startProcessingWorkers()
{
  {
    std::lock_guard<std::mutex> lock(m_processingMutex);

    m_stopProcessing = false;
    m_generation = 0;
    m_completedWorkers = 0;
  }

  m_processingThread1 = std::thread(
      &PipelineWorker::processingWorker, this, 0);

  m_processingThread2 = std::thread(
      &PipelineWorker::processingWorker, this, 1);
}

// ============================================
// Stop and join processing threads
// ============================================

void PipelineWorker::stopProcessingWorkers()
{
  {
    std::lock_guard<std::mutex> lock(m_processingMutex);

    m_stopProcessing = true;
  }

  m_workCV.notify_all();

  if (m_processingThread1.joinable())
    m_processingThread1.join();

  if (m_processingThread2.joinable())
    m_processingThread2.join();
}

// ============================================
// Individual camera processing worker
// ============================================

void PipelineWorker::processingWorker(int cameraIndex)
{
  // Construct a separate processor for this worker.
  // This happens once, not once per frame.
  HSVThreshParams hsvParams = m_hsvParams;

  std::unique_ptr<ImageProcessor> processor;

  try
  {
    std::cout << "Worker " << cameraIndex << ": loading stereo data" << std::endl;

    processor = std::make_unique<ImageProcessor>("core/stereo_data.npz", hsvParams);

    std::cout << "Worker " << cameraIndex << ": ready" << std::endl;
  }
  catch (const std::exception &e)
  {
    // Keep the thread alive (without a processor) so the capture loop
    // never waits forever for this worker to report completion.
    std::cerr << "Worker " << cameraIndex
              << " init failed: " << e.what() << std::endl;
  }

  uint64_t lastGeneration = 0;

  while (true)
  {
    Frame inputFrame;

    // ----------------------------------------
    // Wait for a new frame
    // ----------------------------------------

    {
      std::unique_lock<std::mutex> lock(m_processingMutex);

      m_workCV.wait(lock, [&]()
                    { return m_stopProcessing ||
                             m_generation != lastGeneration; });

      if (m_stopProcessing)
        return;

      lastGeneration = m_generation;

      // Get this worker's input frame
      inputFrame = m_inputFrames[cameraIndex];
    }

    // ----------------------------------------
    // Process the frame
    // ----------------------------------------

    Frame processedFrame;

    try
    {
      // Clone image into independently owned memory
      processedFrame.raw = inputFrame.raw.clone();

      if (processor && !processedFrame.raw.empty())
      {
        // HSV thresholding
        processedFrame.thresh =
            processor->hsvThreshold(processedFrame.raw);

        // Ball detection
        processor->detectBall(
            processedFrame.raw,
            processedFrame.thresh,
            processedFrame.hasBall,
            processedFrame.ballCenter,
            processedFrame.ballRadius,
            processedFrame.annotated);
      }
    }
    catch (const std::exception &e)
    {
      std::cerr << "Processing worker "
                << cameraIndex + 1
                << " error: " << e.what() << std::endl;

      // Preserve the original image if processing fails
      processedFrame.raw = inputFrame.raw.clone();
    }
    catch (...)
    {
      std::cerr << "Processing worker "
                << cameraIndex + 1
                << " unknown error" << std::endl;

      processedFrame.raw = inputFrame.raw.clone();
    }

    // The display code reads these, so never leave them empty
    if (processedFrame.thresh.empty() && !processedFrame.raw.empty())
      processedFrame.thresh = cv::Mat::zeros(
          processedFrame.raw.rows, processedFrame.raw.cols, CV_8UC1);

    if (processedFrame.annotated.empty())
      processedFrame.annotated = processedFrame.raw.clone();

    // Preserve timestamps
    processedFrame.cam_ts = inputFrame.cam_ts;
    processedFrame.program_ts = inputFrame.program_ts;

    // Store this worker's result
    m_processedFrames[cameraIndex] = std::move(processedFrame);

    // ----------------------------------------
    // Notify capture loop that processing finished
    // ----------------------------------------

    {
      std::lock_guard<std::mutex> lock(m_processingMutex);

      ++m_completedWorkers;

      if (m_completedWorkers == 2)
        m_doneCV.notify_one();
    }
  }
}

void PipelineWorker::startCapture()
{
  using Clock = std::chrono::steady_clock;

  try
  {
    m_cams.startCameras();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Camera start failed: " << e.what() << std::endl;
    emit finished();
    return;
  }

  std::cout << "startCapture: cameras up" << std::endl;

  // Start persistent processing threads
  startProcessingWorkers();

  std::cout << "startCapture: workers started, entering loop" << std::endl;

  long i = 0;

  constexpr int GUI_REFRESH_WAIT = 20;
  constexpr int FPS_MEASURE_FRAMES = 20;

  // Camera FPS measurement
  uint64_t fps_start_ts = 0;
  int fps_frame_count = 0;

  // Pipeline timing measurement
  double pipeline_elapsed_ms = 0.0;
  double grab_elapsed_ms = 0.0;

  int pipeline_frame_count = 0;

  while (!QThread::currentThread()->isInterruptionRequested())
  {
    auto loop_start = Clock::now();

    // ----------------------------------------
    // Retrieve stereo frames
    // ----------------------------------------

    auto grab_start = Clock::now();

    std::array<Frame, 2> frames;

    try
    {
      frames = m_cams.grabFrame();
    }
    catch (const std::exception &e)
    {
      std::cerr << "grabFrame failed: " << e.what() << std::endl;
      break;
    }

    auto grab_end = Clock::now();

    double grab_ms =
        std::chrono::duration<double, std::milli>(
            grab_end - grab_start)
            .count();

    // ----------------------------------------
    // Send frames to processing workers
    // ----------------------------------------

    {
      std::lock_guard<std::mutex> lock(m_processingMutex);

      m_inputFrames = frames;

      m_completedWorkers = 0;
      ++m_generation;
    }

    m_workCV.notify_all();

    // ----------------------------------------
    // Wait for both workers to finish
    // ----------------------------------------

    {
      std::unique_lock<std::mutex> lock(m_processingMutex);

      m_doneCV.wait(lock, [this]()
                    { return m_completedWorkers == 2; });
    }

    auto loop_end = Clock::now();

    double loop_ms =
        std::chrono::duration<double, std::milli>(
            loop_end - loop_start)
            .count();

    // ----------------------------------------
    // Camera FPS measurement
    // ----------------------------------------

    uint64_t current_ts = m_processedFrames[0].cam_ts;

    if (current_ts != 0)
    {
      if (fps_frame_count == 0)
      {
        fps_start_ts = current_ts;
        fps_frame_count = 1;
      }
      else
      {
        ++fps_frame_count;

        if (fps_frame_count >= FPS_MEASURE_FRAMES)
        {
          uint64_t elapsed_ns = current_ts - fps_start_ts;

          if (elapsed_ns > 0)
          {
            double camera_fps =
                (fps_frame_count - 1) * 1e9 / elapsed_ns;

            std::cout << "Camera FPS: "
                      << camera_fps << "\n";
          }

          fps_frame_count = 0;
        }
      }
    }

    // ----------------------------------------
    // Pipeline performance measurement
    // ----------------------------------------

    pipeline_elapsed_ms += loop_ms;
    grab_elapsed_ms += grab_ms;

    ++pipeline_frame_count;

    if (pipeline_frame_count >= FPS_MEASURE_FRAMES)
    {
      double average_loop_ms =
          pipeline_elapsed_ms / pipeline_frame_count;

      double average_grab_ms =
          grab_elapsed_ms / pipeline_frame_count;

      double pipeline_fps =
          1000.0 / average_loop_ms;

      std::cout << "Average grab: "
                << average_grab_ms << " ms | "
                << "Average pipeline: "
                << average_loop_ms << " ms | "
                << "Pipeline FPS: "
                << pipeline_fps << "\n";

      pipeline_elapsed_ms = 0.0;
      grab_elapsed_ms = 0.0;
      pipeline_frame_count = 0;
    }

    // ----------------------------------------
    // GUI refresh
    // ----------------------------------------

    if (i % GUI_REFRESH_WAIT == 0)
    {
      emit newFrame(
          m_processedFrames[0],
          m_processedFrames[1]);
    }

    ++i;
  }

  // ----------------------------------------
  // Cleanup
  // ----------------------------------------

  stopProcessingWorkers();

  m_cams.stopCameras();

  emit finished();
}

// ============================================
// Stop capture
// ============================================

void PipelineWorker::stopCapture()
{
  // The capture loop exits when its QThread
  // receives a requestInterruption().
}