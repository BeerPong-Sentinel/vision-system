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

#include "../camera_params.h"

#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <array>


inline const CameraDetails kCam1 = {"Camera_1", "24292737"};
inline const CameraDetails kCam2 = {"Camera_2", "24292752"}; 

// inline const CameraDetails kCam1 = {"Camera L", "25376120"}; // Badminton Bot Team's Other Camera
// inline const CameraDetails kCam2 = {"Camera R", "25376121"}; // Badminton Bot Team's Camera Id

class CameraController {

public:
  CameraController() {};
  ~CameraController();

  void startCameras();
  void stopCameras();

  std::array<Frame, 2> grabFrame();

private:
  Spinnaker::SystemPtr system;
  Spinnaker::CameraList camList;
  Spinnaker::CameraPtr cam1;
  Spinnaker::CameraPtr cam2;
};
