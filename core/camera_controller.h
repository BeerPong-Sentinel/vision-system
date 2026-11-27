#pragma once

// 1. Include the specific file that defines the platform macros first
#include <SpinnakerPlatform.h>

// 2. Fix the broken macro.
// We redefine it to ignore the 'deprecated' attribute and just provide the 'class' keyword.
#undef SPINNAKER_DEPRECATED_CLASS
#define SPINNAKER_DEPRECATED_CLASS(msg) SPINNAKER_API class

// 3. Now include the main library
#include "Spinnaker.h"
#include <SpinGenApi/SpinnakerGenApi.h>

#include "../datatypes.h"

#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <array>
#include "../lib/json.hpp"

#include <fstream>

//Parallel Processing for Frame Acquisition & CV
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <array>

inline const CameraDetails kCam1 = {"Camera_1", "24292737"};
inline const CameraDetails kCam2 = {"Camera_2", "24292752"};

// inline const CameraDetails kCam1 = {"Camera L", "25376120"}; // Badminton Bot Team's Other Camera
// inline const CameraDetails kCam2 = {"Camera R", "25376121"}; // Badminton Bot Team's Camera Id

class CameraController
{

public:
  CameraController(CameraParams params = {});
  ~CameraController();

  void startCameras();
  void stopCameras();

  std::array<Frame, 2> grabFrame();

private:
  Spinnaker::SystemPtr system;
  Spinnaker::CameraList camList;
  Spinnaker::CameraPtr cam1;
  Spinnaker::CameraPtr cam2;

  CameraParams m_params;
  std::chrono::steady_clock::time_point m_program_start = std::chrono::steady_clock::now();
  std::ofstream frame_logs{"logs/frames.json"};

  // Persistent camera worker threads
  std::thread m_cam1Thread;
  std::thread m_cam2Thread;

  // Thread synchronization
  std::mutex m_mutex;
  std::condition_variable m_workCV;
  std::condition_variable m_doneCV;

  bool m_stopWorkers = false;

  uint64_t m_generation = 0;
  int m_completedWorkers = 0;

  // Frames produced by the workers
  std::array<Frame, 2> m_workerFrames;

  // Worker functions
  void startWorkers();
  void stopWorkers();
  void cameraWorker(int cameraIndex);
};
