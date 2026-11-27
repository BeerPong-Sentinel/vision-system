#include "pipeline_worker.h"

#include <opencv2/opencv.hpp>

#include <algorithm>
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

    processor = std::make_unique<ImageProcessor>(
        "core/stereo_data.npz", hsvParams);

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
        using Clock = std::chrono::steady_clock;

        // HSV thresholding
        auto thresh_start = Clock::now();

        processedFrame.thresh =
            processor->hsvThreshold(processedFrame.raw);

        auto thresh_end = Clock::now();

        // Ball detection
        processor->detectBall(
            processedFrame.raw,
            processedFrame.thresh,
            processedFrame.hasBall,
            processedFrame.ballCenter,
            processedFrame.ballRadius,
            processedFrame.annotated);

        auto detect_end = Clock::now();

        m_thresholdMs[cameraIndex] =
            std::chrono::duration<double, std::milli>(
                thresh_end - thresh_start)
                .count();

        m_detectMs[cameraIndex] =
            std::chrono::duration<double, std::milli>(
                detect_end - thresh_end)
                .count();
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
    // GUI refresh
    // ----------------------------------------

    if (i % GUI_REFRESH_WAIT == 0)
    {
      // The loop waits for the slower camera, so report the slower
      // of the two workers for each stage.
      double threshold_ms = std::max(m_thresholdMs[0], m_thresholdMs[1]);
      double detect_ms = std::max(m_detectMs[0], m_detectMs[1]);
      double fps = (loop_ms > 0.0) ? 1000.0 / loop_ms : 0.0;

      std::cout << "Grab: " << grab_ms << " ms | "
                << "Threshold: " << threshold_ms << " ms | "
                << "BallDetect: " << detect_ms << " ms | "
                << "Total: " << loop_ms << " ms | "
                << "FPS: " << fps << "\n";

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