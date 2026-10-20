#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QtCore/QDebug>
#include <opencv2/opencv.hpp>
#include <format>
#include "yaml-cpp/yaml.h"
#include <filesystem>

MainWindow::MainWindow(QWidget *parent, ConfigData config)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      camera_worker(this, config.cam),
      image_processing_worker(this, config.hsv)
{

  ui->setupUi(this);
  ui->hueMin->setValue(config.hsv.h_min);
  ui->hueMax->setValue(config.hsv.h_max);
  ui->saturationMin->setValue(config.hsv.s_min);
  ui->saturationMax->setValue(config.hsv.s_max);
  ui->valueMin->setValue(config.hsv.v_min);
  ui->valueMax->setValue(config.hsv.v_max);

  qDebug() << "Connecting Labels";

  connect(&camera_worker, &CameraWorker::newFrame, &image_processing_worker, &ImageProcessorWorker::processFrames);
  connect(&image_processing_worker, &ImageProcessorWorker::newProcessedFrame, this, &MainWindow::updateCameraDisplay);
  connect(&image_processing_worker, &ImageProcessorWorker::newBallPosition, this, &MainWindow::updateBallPosition);
  connect(ui->cameraCapture, &QPushButton::clicked, this, &MainWindow::captureImage);

   auto updateHSV = [this]()
    {
      HSVThreshParams new_params = {
        ui->hueMin->value(),
        ui->hueMax->value(),
        ui->saturationMin->value(),
        ui->saturationMax->value(),
        ui->valueMin->value(),
        ui->valueMax->value()
      };
      image_processing_worker.updateHSVParams(new_params);
    };
    connect(ui->hueMin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateHSV);
    connect(ui->hueMax, QOverload<int>::of(&QSpinBox::valueChanged), this, updateHSV);
    connect(ui->saturationMin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateHSV);
    connect(ui->saturationMax, QOverload<int>::of(&QSpinBox::valueChanged), this, updateHSV);
    connect(ui->valueMin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateHSV);
    connect(ui->valueMax, QOverload<int>::of(&QSpinBox::valueChanged), this, updateHSV);
  qDebug() << "Starting Cameras";

  connect(ui->margin_val, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]()
          { image_processing_worker.updateThreshParams(ui->margin_val->value(), ui->low_thresh_val->value(), ui->min_size_val->value(), ui->margin_val_2->value()); });

  camera_worker.start(config);

  ui->captureNum->setText(QString::number(cameraCaptureNum));
}

MainWindow::~MainWindow()
{

}

void MainWindow::updateCameraDisplay(const Frame &frame1, const Frame &frame2)
{
  int index = ui->imagedisptype->currentIndex();
  
  // std::cout << "Cam 1 cam_ts: " << frame1.cam_ts << std::endl;
  // std::cout << "Cam 1 program_ts: " << frame1.program_ts << std::endl;

  QImage qimg1, qimg2;
  latestFrame1 = frame1.raw.clone();
  latestFrame2 = frame2.raw.clone();

  // Decide Which Image to Display
  if (index == 0)
  {
    qimg1 = QImage(frame1.raw.data, frame1.raw.cols, frame1.raw.rows, static_cast<int>(frame1.raw.step), QImage::Format_RGB888).copy();
    qimg2 = QImage(frame2.raw.data, frame2.raw.cols, frame2.raw.rows, static_cast<int>(frame2.raw.step), QImage::Format_RGB888).copy();
  }
  else if (index == 1)
  { // Threshold Case
    qimg1 = QImage(frame1.thresh.data, frame1.thresh.cols, frame1.thresh.rows, static_cast<int>(frame1.thresh.step), QImage::Format_Grayscale8).copy();
    qimg2 = QImage(frame2.thresh.data, frame2.thresh.cols, frame2.thresh.rows, static_cast<int>(frame2.thresh.step), QImage::Format_Grayscale8).copy();
  }
  else if (index == 2)
  { // Ball detection case
    qimg1 = QImage(frame1.annotated.data, frame1.annotated.cols, frame1.annotated.rows, static_cast<int>(frame1.annotated.step), QImage::Format_RGB888).copy();
    qimg2 = QImage(frame2.annotated.data, frame2.annotated.cols, frame2.annotated.rows, static_cast<int>(frame2.annotated.step), QImage::Format_RGB888).copy();
    if (frame1.hasBall)
    {
      ui->size_label->setText(QString::number(frame1.ballRadius * 2, 'f', 1));
    }
    else
    {
      ui->size_label->setText("--");
    }
  }

  // Convert images to QPixmap and send to QLabel
  ui->camera1->setPixmap(QPixmap::fromImage(qimg1).scaled(ui->camera1->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  ui->camera2->setPixmap(QPixmap::fromImage(qimg2).scaled(ui->camera2->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::updateBallPosition(std::optional<cv::Point3f> pos) {
  if (pos.has_value()) {
    ui->ball_pos->setText(QString::fromStdString(std::format("({:.2f}, {:.2f}, {:.2})", pos.value().x, pos.value().y, pos.value().z)));
  } else {
    ui->ball_pos->setText("(__, __, __)");
  }
}

void MainWindow::captureImage()
{
  namespace fs = std::filesystem;
  const fs::path dir = fs::path(PROJECT_DIR) / "CalibrationImages";
  fs::create_directories(dir);

  std::stringstream ss_1;
  std::stringstream ss_2;
  ss_1 << (dir / kCam1.name).string() << "_" << cameraCaptureNum << ".png";
  ss_2 << (dir / kCam2.name).string() << "_" << cameraCaptureNum << ".png";



  if (!cv::imwrite(ss_1.str(), latestFrame1))
    qDebug() << "Failed to write" << QString::fromStdString(ss_1.str());
  if (!cv::imwrite(ss_2.str(), latestFrame2))
    qDebug() << "Failed to write" << QString::fromStdString(ss_2.str());

  cameraCaptureNum++;
  ui->captureNum->setText(QString::number(cameraCaptureNum));
}