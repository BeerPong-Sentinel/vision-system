#pragma once
#include <QMainWindow>
#include <opencv2/opencv.hpp>

// Import Qt Nodes
#include "camera_worker.h"
#include "image_processing.h"
#include "camera_worker.h"

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
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void updateCameraDisplay(const Frame &frame1, const Frame &frame2);

private:
  Ui::MainWindow *ui;
  std::unique_ptr<CameraWorker> camera_worker;
  ImageProcessor *imageProcessor;
};
