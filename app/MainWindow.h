#pragma once
#include <QMainWindow>
#include <opencv2/opencv.hpp>

// Import Qt Nodes
#include "camera_worker.h"
#include "image_processing_worker.h"
#include "pipeline_worker.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
  class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr, ConfigData config = {});
  ~MainWindow();

private slots:
  void updateCameraDisplay(const Frame &frame1, const Frame &frame2);
  void updateBallPosition(std::optional<cv::Point3f> pos);
  void captureImage();

private:
  void pipelineStart();
  void pipelineStop();

  Ui::MainWindow *ui;
  CameraWorker camera_worker;
  ImageProcessorWorker image_processing_worker;
  PipelineWorker pipeline_worker;
  cv::Mat latestFrame1, latestFrame2;
  int cameraCaptureNum = 0;

  QThread m_pipelineThread;
};
