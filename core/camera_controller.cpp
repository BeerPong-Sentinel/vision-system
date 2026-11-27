#include "camera_controller.h"
#include <stdexcept>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

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

void CameraController::startWorkers()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stopWorkers = false;
    m_generation = 0;
    m_completedWorkers = 0;
  }

  m_cam1Thread = std::thread(&CameraController::cameraWorker, this, 0);
  m_cam2Thread = std::thread(&CameraController::cameraWorker, this, 1);
}

void CameraController::stopWorkers()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stopWorkers = true;
  }

  m_workCV.notify_all();

  if (m_cam1Thread.joinable())
  {
    m_cam1Thread.join();
  }

  if (m_cam2Thread.joinable())
  {
    m_cam2Thread.join();
  }
}

void CameraController::cameraWorker(int cameraIndex)
{
  //using Clock = std::chrono::steady_clock;

  // Each worker gets its own camera and image processor
  Spinnaker::CameraPtr camera = (cameraIndex == 0) ? cam1 : cam2;

  Spinnaker::ImageProcessor processor;

  // Fastest demosaic; slightly lower colour quality at edges, which does
  // not matter for thresholding. Check the enum name in your SDK headers.
  processor.SetColorProcessing(
      Spinnaker::SPINNAKER_COLOR_PROCESSING_ALGORITHM_NEAREST_NEIGHBOR);

  uint64_t lastGeneration = 0;

  while (true)
  {
    // Wait for the next capture request
    {
      std::unique_lock<std::mutex> lock(m_mutex);

      m_workCV.wait(lock, [&]()
                    { return m_stopWorkers || m_generation != lastGeneration; });

      if (m_stopWorkers)
      {
        return;
      }

      lastGeneration = m_generation;
    }

    Frame &frame = m_workerFrames[cameraIndex];

    frame.cam_ts = 0;
    frame.program_ts = 0;

    bool gotImage = false;

    Spinnaker::ImagePtr image;

    try
    {
      image = camera->GetNextImage(500);

      if (image->IsIncomplete())
      {
        std::cerr << "Image" << cameraIndex + 1
                  << " Incomplete: "
                  << Spinnaker::Image::GetImageStatusDescription(
                         image->GetImageStatus())
                  << std::endl;
      }
      else
      {
        Spinnaker::ImagePtr convertedImage = processor.Convert(image, Spinnaker::PixelFormat_RGB8);

        cv::Mat tmp(
            convertedImage->GetHeight(),
            convertedImage->GetWidth(),
            CV_8UC3,
            convertedImage->GetData());

        // Retrieve camera timestamp
        frame.cam_ts = image->GetChunkData().GetTimestamp();

        // Copy image into independently owned OpenCV memory
        frame.raw = tmp.clone();

        gotImage = true;
      }
    }
    catch (const Spinnaker::Exception &e)
    {
      std::cerr << "Cam" << cameraIndex + 1
                << " error: " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Cam" << cameraIndex + 1
                << " error: " << e.what() << std::endl;
    }

    // Only build the black fallback frame when acquisition failed
    if (!gotImage)
    {
      frame.raw = cv::Mat(540, 720, CV_8UC3, cv::Scalar(0, 0, 0));
    }

    // Release the image even if conversion throws
    if (image)
    {
      image->Release();
    }

    // Signal that this worker has completed its capture
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      ++m_completedWorkers;

      if (m_completedWorkers == 2)
        m_doneCV.notify_one();
    }
  }
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

  // Start persistent workers after both cameras begin acquisition
  if (cam1 && cam2)
  {
    startWorkers();
  }
  else
  {
    throw std::runtime_error("Could not initialize both cameras");
  }
}

void CameraController::stopCameras()
{
  std::cout << "Ending Acquisition" << std::endl;
  stopWorkers();
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
  //using Clock = std::chrono::steady_clock;
  //auto total_start = Clock::now();

  // Start a new capture cycle
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_completedWorkers = 0;
    ++m_generation;
  }

  // Wake both camera workers
  m_workCV.notify_all();

  // Wait until both cameras finish retrieving and processing
  {
    std::unique_lock<std::mutex> lock(m_mutex);

    m_doneCV.wait(lock, [this]()
                  { return m_completedWorkers == 2; });
  }

  /*
  auto total_end = Clock::now();

  double total_ms = std::chrono::duration<double, std::milli>( total_end - total_start).count();

  
  std::cout << "Total grabFrame: "
            << total_ms
            << " ms | Effective FPS: "
            << 1000.0 / total_ms
            << "\n";
  
*/
  return m_workerFrames;
}