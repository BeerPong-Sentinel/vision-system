#include "camera_controller.h"
#include <stdexcept>

namespace {
  void ConfigureChunkData(Spinnaker::GenApi::INodeMap& nodeMap) {
    std::cout << "Configuring Chunk Data" << std::endl;
    Spinnaker::GenApi::CBooleanPtr ptrChunkModeActive = nodeMap.GetNode("ChunkModeActive");

    if (!IsWritable(ptrChunkModeActive)) {
      throw std::runtime_error("Couldn't activate Chunk Mode");
    }

    ptrChunkModeActive->SetValue(true);

    // Retrieve the selector node
    Spinnaker::GenApi::CEnumerationPtr ptrChunkSelector = nodeMap.GetNode("ChunkSelector");
    if (!IsReadable(ptrChunkSelector)) {
      throw std::runtime_error("Couldn't retrieve Chunk Selector");
    }
    
    Spinnaker::GenApi::CEnumEntryPtr timestampEntry = ptrChunkSelector->GetEntryByName("Timestamp");

    if (!IsReadable(timestampEntry)) {
      throw std::runtime_error("Timestamp chunk not available");
    }

    ptrChunkSelector->SetIntValue(timestampEntry->GetValue());
     
    Spinnaker::GenApi::CBooleanPtr chunkEnable = nodeMap.GetNode("ChunkEnable");

    if (!IsAvailable(chunkEnable) || !IsWritable(chunkEnable)) {
      throw std::runtime_error("ChunkEnable not writable.");
    }

    chunkEnable->SetValue(true);
  }
  void InitializeCam(Spinnaker::CameraPtr& cam, CameraDetails kCam, const Spinnaker::CameraList& camList) {
    cam= camList.GetBySerial(kCam.deviceID);

    if (cam)
    {
      cam->Init();
      
      Spinnaker::GenApi::INodeMap& sNodeMap = cam->GetTLStreamNodeMap();
      Spinnaker::GenApi::INodeMap& nodeMap = cam->GetNodeMap();
      ConfigureChunkData(nodeMap);
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

      const Spinnaker::ChunkData& chunkData = cam1frame->GetChunkData(); 

      // Timestamp is in nanoseconds
      int64_t cam_ts = chunkData.GetTimestamp();
      auto program_time = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(program_time - m_program_start);

      frames.at(0).raw = tmp.clone();
      frames.at(0).cam_ts = cam_ts;
      frames.at(0).program_ts = static_cast<int64_t>(duration.count());
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

      const Spinnaker::ChunkData& chunkData = cam2frame->GetChunkData(); 

      // Timestamp is in nanoseconds
      int64_t cam_ts = chunkData.GetTimestamp();

      auto program_time = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(program_time - m_program_start);

      frames.at(1).raw = tmp.clone();
      frames.at(1).cam_ts = cam_ts;
      frames.at(1).program_ts = static_cast<int64_t>(duration.count());
    }

    cam2frame->Release();
  }
  
  return frames;
}
