#include "camera_controller.h"
#include <stdexcept>
#include <chrono>
#include <future>

namespace
{
  void ConfigureChunkData(Spinnaker::GenApi::INodeMap &nodeMap)
  {
    std::cout << "Configuring Chunk Data" << std::endl;
    Spinnaker::GenApi::CBooleanPtr ptrChunkModeActive = nodeMap.GetNode("ChunkModeActive");

    if (!IsWritable(ptrChunkModeActive))
    {
      throw std::runtime_error("Couldn't activate Chunk Mode");
    }

    ptrChunkModeActive->SetValue(true);

    // Retrieve the selector node
    Spinnaker::GenApi::CEnumerationPtr ptrChunkSelector = nodeMap.GetNode("ChunkSelector");
    if (!IsReadable(ptrChunkSelector))
    {
      throw std::runtime_error("Couldn't retrieve Chunk Selector");
    }

    Spinnaker::GenApi::CEnumEntryPtr timestampEntry = ptrChunkSelector->GetEntryByName("Timestamp");

    if (!IsReadable(timestampEntry))
    {
      throw std::runtime_error("Timestamp chunk not available");
    }

    ptrChunkSelector->SetIntValue(timestampEntry->GetValue());

    Spinnaker::GenApi::CBooleanPtr chunkEnable = nodeMap.GetNode("ChunkEnable");

    if (!IsAvailable(chunkEnable) || !IsWritable(chunkEnable))
    {
      throw std::runtime_error("ChunkEnable not writable.");
    }

    chunkEnable->SetValue(true);
  }

  void ConfigureCameraControlsESP(Spinnaker::GenApi::INodeMap &nodeMap, CameraParams config)
  {
    // Disable automatic exposure.
    Spinnaker::GenApi::CEnumerationPtr exposureAuto = nodeMap.GetNode("ExposureAuto");

    if (IsAvailable(exposureAuto) && IsWritable(exposureAuto))
    {
      Spinnaker::GenApi::CEnumEntryPtr off = exposureAuto->GetEntryByName("Off");

      if (IsAvailable(off) && IsReadable(off))
      {
        exposureAuto->SetIntValue(off->GetValue());
      }
      else
      {
        throw std::runtime_error("ExposureAuto Off is unavailable");
      }
    }

    // Perform automatic white balance once, then lock the gains.
    Spinnaker::GenApi::CEnumerationPtr balanceWhiteAuto = nodeMap.GetNode("BalanceWhiteAuto");

    if (!IsAvailable(balanceWhiteAuto) || !IsWritable(balanceWhiteAuto))
      throw std::runtime_error("BalanceWhiteAuto is unavailable");

    Spinnaker::GenApi::CEnumEntryPtr once =
        balanceWhiteAuto->GetEntryByName("Once");

    if (!IsAvailable(once) || !IsReadable(once))
      throw std::runtime_error("BalanceWhiteAuto Once is unavailable");

    balanceWhiteAuto->SetIntValue(once->GetValue());

    // Set exposure time in microseconds.
    Spinnaker::GenApi::CFloatPtr exposure = nodeMap.GetNode("ExposureTime");

    if (!IsAvailable(exposure) || !IsWritable(exposure))
    {
      throw std::runtime_error("ExposureTime is not writable");
    }

    if (config.exposure_time < exposure->GetMin() || config.exposure_time > exposure->GetMax())
    {
      throw std::out_of_range("Exposure time is outside camera limits");
    }

    exposure->SetValue(config.exposure_time);

    // Set Trigger Mode to Off for configuration
    Spinnaker::GenApi::CEnumerationPtr triggerMode = nodeMap.GetNode("TriggerMode");
    if (!IsAvailable(triggerMode) || !IsWritable(triggerMode))
    {
      throw std::runtime_error("TriggerMode is not writable");
    }

    triggerMode->SetIntValue(triggerMode->GetEntryByName("Off")->GetValue());

    // Select frame start as the trigger event
    Spinnaker::GenApi::CEnumerationPtr triggerSelector = nodeMap.GetNode("TriggerSelector");

    if (!IsAvailable(triggerSelector) || !IsWritable(triggerSelector))
    {
      throw std::runtime_error("TriggerSelector unavailable");
    }

    triggerSelector->SetIntValue(
        triggerSelector->GetEntryByName("FrameStart")->GetValue());

    // Enable trigger overlap during readout
    Spinnaker::GenApi::CEnumerationPtr triggerOverlap = nodeMap.GetNode("TriggerOverlap");

    if (!IsAvailable(triggerOverlap) || !IsWritable(triggerOverlap))
    {
      throw std::runtime_error("TriggerOverlap is not writable");
    }

    Spinnaker::GenApi::CEnumEntryPtr readOut = triggerOverlap->GetEntryByName("ReadOut");

    if (!IsAvailable(readOut) || !IsReadable(readOut))
    {
      throw std::runtime_error("TriggerOverlap ReadOut is unavailable");
    }

    triggerOverlap->SetIntValue(readOut->GetValue());

    // Set trigger source to Line0
    Spinnaker::GenApi::CEnumerationPtr triggerSource = nodeMap.GetNode("TriggerSource");
    if (!IsAvailable(triggerSource) || !IsWritable(triggerSource))
    {
      throw std::runtime_error("TriggerSource is not writable");
    }
    triggerSource->SetIntValue(triggerSource->GetEntryByName("Line0")->GetValue());

    // Set trigger activation to Rising Edge
    Spinnaker::GenApi::CEnumerationPtr triggerActivation = nodeMap.GetNode("TriggerActivation");
    if (!IsAvailable(triggerActivation) || !IsWritable(triggerActivation))
    {
      throw std::runtime_error("TriggerActivation is not writable");
    }
    triggerActivation->SetIntValue(triggerActivation->GetEntryByName("RisingEdge")->GetValue());

    // Start trigger on Line0
    triggerMode->SetIntValue(triggerMode->GetEntryByName("On")->GetValue());

  }

  // Software control of camera parameters; kept for testing
  void ConfigureCameraControlsPC(Spinnaker::GenApi::INodeMap &nodeMap, CameraParams config)
  {
    // Disable automatic exposure.
    Spinnaker::GenApi::CEnumerationPtr exposureAuto = nodeMap.GetNode("ExposureAuto");

    if (IsAvailable(exposureAuto) && IsWritable(exposureAuto))
    {
      Spinnaker::GenApi::CEnumEntryPtr off = exposureAuto->GetEntryByName("Off");

      if (IsAvailable(off) && IsReadable(off))
      {
        exposureAuto->SetIntValue(off->GetValue());
      }
      else
      {
        throw std::runtime_error("ExposureAuto Off is unavailable");
      }
    }

    // Set exposure time in microseconds.
    Spinnaker::GenApi::CFloatPtr exposure = nodeMap.GetNode("ExposureTime");

    if (!IsAvailable(exposure) || !IsWritable(exposure))
    {
      throw std::runtime_error("ExposureTime is not writable");
    }

    if (config.exposure_time < exposure->GetMin() || config.exposure_time > exposure->GetMax())
    {
      throw std::out_of_range("Exposure time is outside camera limits");
    }

    exposure->SetValue(config.exposure_time);

    // Enable manual frame-rate control.
    Spinnaker::GenApi::CBooleanPtr frameRateEnable = nodeMap.GetNode("AcquisitionFrameRateEnable");

    if (IsAvailable(frameRateEnable) && IsWritable(frameRateEnable))
    {
      frameRateEnable->SetValue(true);
    }

    // Set requested FPS.

    Spinnaker::GenApi::CFloatPtr frameRate = nodeMap.GetNode("AcquisitionFrameRate");

    if (!IsAvailable(frameRate) || !IsWritable(frameRate))
    {
      throw std::runtime_error(
          "AcquisitionFrameRate is not writable");
    }

    if (config.fps < frameRate->GetMin() || config.fps > frameRate->GetMax())
    {
      throw std::out_of_range("FPS is outside camera limits");
    }

    frameRate->SetValue(config.fps);

    std::cout << "Camera controls configured: "
              << exposure->GetValue() << " us exposure, "
              << frameRate->GetValue() << " FPS"
              << std::endl;
  }

  void InitializeCamESP(Spinnaker::CameraPtr &cam, CameraDetails kCam, const Spinnaker::CameraList &camList, CameraParams config)
  {
    cam = camList.GetBySerial(kCam.deviceID);

    if (cam)
    {
      cam->Init();

      Spinnaker::GenApi::INodeMap &sNodeMap = cam->GetTLStreamNodeMap();
      Spinnaker::GenApi::INodeMap &nodeMap = cam->GetNodeMap();
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

      ConfigureCameraControlsESP(nodeMap, config);

      cam->BeginAcquisition();

      std::cout << "Successfully Initialized" << kCam.name << std::endl;
    }
    else
    {
      std::cout << "Could Not Connect to " << kCam.name << std::endl;
    }
  }

}

// Initialize using software control of camera parameters; kept for testing
void InitializeCamPC(Spinnaker::CameraPtr &cam, CameraDetails kCam, const Spinnaker::CameraList &camList, CameraParams config)
{
  cam = camList.GetBySerial(kCam.deviceID);

  if (cam)
  {
    cam->Init();

    Spinnaker::GenApi::INodeMap &sNodeMap = cam->GetTLStreamNodeMap();
    Spinnaker::GenApi::INodeMap &nodeMap = cam->GetNodeMap();
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

    ConfigureCameraControlsPC(nodeMap, config);

    cam->BeginAcquisition();

    std::cout << "Successfully Initialized" << kCam.name << std::endl;
  }
  else
  {
    std::cout << "Could Not Connect to " << kCam.name << std::endl;
  }
}

CameraController::CameraController(CameraParams params) : m_params(params)
{
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

  InitializeCamESP(cam1, kCam1, camList, m_params);
  InitializeCamESP(cam2, kCam2, camList, m_params);
}

void CameraController::stopCameras()
{
  std::cout << "Ending Acquisition" << std::endl;
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

  if (system)
  {
    system->ReleaseInstance();
    system = nullptr;
  }
}

std::array<Frame, 2> CameraController::grabFrame()
{
    using Clock = std::chrono::steady_clock;

    auto total_start = Clock::now();

    const int image_width = 720;
    const int image_height = 540;

    std::array<Frame, 2> frames;

    // Initialize default black frames
    frames[0].raw = cv::Mat(
        image_height, image_width, CV_8UC3, cv::Scalar(0, 0, 0));

    frames[1].raw = cv::Mat(
        image_height, image_width, CV_8UC3, cv::Scalar(0, 0, 0));

    // Retrieve both cameras in parallel
    auto retrieve_start = Clock::now();

    auto cam1_future = std::async(std::launch::async, [this]()
    {
        return cam1 ? cam1->GetNextImage(500) : Spinnaker::ImagePtr(nullptr);
    });

    auto cam2_future = std::async(std::launch::async, [this]()
    {
        return cam2 ? cam2->GetNextImage(500) : Spinnaker::ImagePtr(nullptr);
    });

    Spinnaker::ImagePtr cam1frame = cam1_future.get();
    Spinnaker::ImagePtr cam2frame = cam2_future.get();

    auto retrieve_end = Clock::now();

    double retrieve_ms =
        std::chrono::duration<double, std::milli>(
            retrieve_end - retrieve_start).count();

    // Reuse image processor
    Spinnaker::ImageProcessor processor;

    // Convert and copy Camera 1
    if (cam1frame)
    {
        if (cam1frame->IsIncomplete())
        {
            std::cerr << "Image1 Incomplete: "
                      << Spinnaker::Image::GetImageStatusDescription(
                             cam1frame->GetImageStatus())
                      << std::endl;
        }
        else
        {
            auto convert_start = Clock::now();

            Spinnaker::ImagePtr convertedImage =
                processor.Convert(cam1frame, Spinnaker::PixelFormat_RGB8);

            cv::Mat tmp(
                convertedImage->GetHeight(),
                convertedImage->GetWidth(),
                CV_8UC3,
                convertedImage->GetData());

            auto convert_end = Clock::now();

            const Spinnaker::ChunkData& chunkData =
                cam1frame->GetChunkData();

            frames[0].cam_ts = chunkData.GetTimestamp();

            auto program_time = Clock::now();

            auto program_duration =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    program_time - m_program_start);

            frames[0].raw = tmp.clone();

            frames[0].program_ts =
                static_cast<int64_t>(program_duration.count());

            auto copy_end = Clock::now();

            /*
            std::cout << "Cam1 | Convert: "
                      << std::chrono::duration<double, std::milli>(
                             convert_end - convert_start).count()
                      << " ms | Copy: "
                      << std::chrono::duration<double, std::milli>(
                             copy_end - convert_end).count()
                      << " ms\n";
            */
        }

        cam1frame->Release();
    }

    // Convert and copy Camera 2
    if (cam2frame)
    {
        if (cam2frame->IsIncomplete())
        {
            std::cerr << "Image2 Incomplete: "
                      << Spinnaker::Image::GetImageStatusDescription(
                             cam2frame->GetImageStatus())
                      << std::endl;
        }
        else
        {
            auto convert_start = Clock::now();

            Spinnaker::ImagePtr convertedImage =
                processor.Convert(cam2frame, Spinnaker::PixelFormat_RGB8);

            cv::Mat tmp(
                convertedImage->GetHeight(),
                convertedImage->GetWidth(),
                CV_8UC3,
                convertedImage->GetData());

            auto convert_end = Clock::now();

            const Spinnaker::ChunkData& chunkData =
                cam2frame->GetChunkData();

            frames[1].cam_ts = chunkData.GetTimestamp();

            auto program_time = Clock::now();

            auto program_duration =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    program_time - m_program_start);

            frames[1].raw = tmp.clone();

            frames[1].program_ts =
                static_cast<int64_t>(program_duration.count());

            auto copy_end = Clock::now();
            /*
            std::cout << "Cam2 | Convert: "
                      << std::chrono::duration<double, std::milli>(
                             convert_end - convert_start).count()
                      << " ms | Copy: "
                      << std::chrono::duration<double, std::milli>(
                             copy_end - convert_end).count()
                      << " ms\n";
            */
        }

        cam2frame->Release();
    }

    auto total_end = Clock::now();

    double total_ms =
        std::chrono::duration<double, std::milli>(
            total_end - total_start).count();

    /*
    std::cout << "Parallel retrieval: " << retrieve_ms
              << " ms | Total grabFrame: " << total_ms
              << " ms | Effective FPS: " << 1000.0 / total_ms
              << "\n";
    */
    return frames;
}