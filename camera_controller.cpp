#include "camera_controller.h"

namespace {
  void InitializeCam(Spinnaker::CameraPtr& cam, CameraDetails kCam, const Spinnaker::CameraList& camList) {
    cam= camList.GetBySerial(kCam.deviceID);

    if (cam)
    {
      cam->Init();

      Spinnaker::GenApi::INodeMap& sNodeMap = cam->GetTLStreamNodeMap();

      Spinnaker::GenApi::CEnumerationPtr ptrHandlingMode = sNodeMap.GetNode("StreamBufferHandlingMode");
      if (IsAvailable(ptrHandlingMode) && IsWritable(ptrHandlingMode))
      {
          Spinnaker::GenApi::CEnumEntryPtr ptrHandlingModeEntry = ptrHandlingMode->GetEntryByName("NewestOnly");
          if (IsAvailable(ptrHandlingModeEntry) && IsReadable(ptrHandlingModeEntry))
          {
              ptrHandlingMode->SetIntValue(ptrHandlingModeEntry->GetValue());
          }
      }
      cam->BeginAcquisition();

      std::cout << "Successfully Initialized" << kCam.name << std::endl;
    }
    else
    {
      std::cout << "Could Not Connect to " << kCam.name << std::endl;
    }
  }
}


CameraController::~CameraController()
{
  stopCameras();
}

void CameraController::startCameras()
{
  // Retrieve singleton reference to system object
  system = Spinnaker::System::GetInstance();

  // Retrieve list of cameras from the system
  camList = system->GetCameras();

  const unsigned int numCameras = camList.GetSize();

  std::cout << "Number of Cameras Detected: " << numCameras << std::endl;

  if (numCameras == 0)
  {
    std::cerr << "Not enough cameras!" << std::endl;
  }
  
  InitializeCam(cam1, kCam1, camList);
  InitializeCam(cam2, kCam2, camList);

}

void CameraController::stopCameras()
{
  camList.Clear();
  if (cam1 && cam1->IsStreaming())
    cam1->EndAcquisition();
  if (cam1 && cam1->IsInitialized())
    cam1->DeInit();

  if (cam2 && cam2->IsStreaming())
    cam2->EndAcquisition();
  if (cam2 && cam2->IsInitialized())
    cam2->DeInit();

  cam1 = nullptr;
  cam2 = nullptr;

  system->ReleaseInstance();
}

std::array<Frame, 2> CameraController::grabFrame()
{
  const int image_width = 720;
  const int image_height = 540;

  Frame frame1;
  Frame frame2;

  // Initialize Default Image (If No Camera Present)
  frame1.raw = cv::Mat(image_height, image_width, CV_8UC3, cv::Scalar(0, 0, 0));
  frame2.raw = cv::Mat(image_height, image_width, CV_8UC3, cv::Scalar(0, 0, 0));

  std::array<Frame, 2> frames = {frame1, frame2};

  if (cam1)
  {

    // Acquire Image
    Spinnaker::ImageProcessor processor;
    Spinnaker::ImagePtr cam1frame = cam1->GetNextImage(1000);

    if (cam1frame->IsIncomplete())
    {
      std::cerr << "Image1 Incomplete: " << Spinnaker::Image::GetImageStatusDescription(cam1frame->GetImageStatus()) << std::endl;
    }
    else
    {
      Spinnaker::ImagePtr convertedImage = processor.Convert(cam1frame, Spinnaker::PixelFormat_RGB8);

      cv::Mat tmp(
          convertedImage->GetHeight(),
          convertedImage->GetWidth(),
          CV_8UC3,
          convertedImage->GetData());
      frames.at(0).raw = tmp.clone();
    }

    cam1frame->Release();
  }

  if (cam2)
  {
    Spinnaker::ImageProcessor processor;
    Spinnaker::ImagePtr cam2frame = cam2->GetNextImage(1000);

    if (cam2frame->IsIncomplete())
    {
      std::cerr << "Image2 Incomplete: " << Spinnaker::Image::GetImageStatusDescription(cam2frame->GetImageStatus());
    }
    else
    {
      Spinnaker::ImagePtr convertedImage = processor.Convert(cam2frame, Spinnaker::PixelFormat_RGB8);

      cv::Mat tmp(
          convertedImage->GetHeight(),
          convertedImage->GetWidth(),
          CV_8UC3,
          convertedImage->GetData());
      frames.at(1).raw = tmp.clone();
    }
    cam2frame->Release();
  }
  
  return frames;
}
