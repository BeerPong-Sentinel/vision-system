#pragma once
#include <QMainWindow>
#include <opencv2/opencv.hpp>

// Import Qt Nodes
#include "CameraController.h"
#include "ImageProcessing.h"
#include "CameraParams.h"

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
  CameraController *cameras;
  ImageProcessor *imageProcessor;
};
