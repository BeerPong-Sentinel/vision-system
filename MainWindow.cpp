#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QtCore/QDebug>
#include <opencv2/opencv.hpp>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{

  ui->setupUi(this);

  qDebug() << "Connecting Labels";

  camera_worker = std::make_unique<CameraWorker>(this);
  imageProcessor = new ImageProcessor(this);

  connect(camera_worker.get(), &CameraWorker::newFrame, imageProcessor, &ImageProcessor::processFrames);
  connect(imageProcessor, &ImageProcessor::newProcessedFrame, this, &MainWindow::updateCameraDisplay);
  connect(ui->margin_val, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]()
          { imageProcessor->updateThreshParams(ui->margin_val->value(), ui->low_thresh_val->value(), ui->min_size_val->value(), ui->margin_val_2->value()); });
  connect(ui->low_thresh_val, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]()
          { imageProcessor->updateThreshParams(ui->margin_val->value(), ui->low_thresh_val->value(), ui->min_size_val->value(), ui->margin_val_2->value()); });
  connect(ui->min_size_val, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]()
          {imageProcessor->updateThreshParams(ui->margin_val->value(), ui->low_thresh_val->value(), ui->min_size_val->value(), ui->margin_val_2->value()); });
  connect(ui->margin_val_2, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]()
          { imageProcessor->updateThreshParams(ui->margin_val->value(), ui->low_thresh_val->value(), ui->min_size_val->value(), ui->margin_val_2->value()); });
  qDebug() << "Starting Cameras";

  camera_worker->start();
}

MainWindow::~MainWindow()
{
}

void MainWindow::updateCameraDisplay(const Frame &frame1, const Frame &frame2)
{
  int index = ui->imagedisptype->currentIndex();
  
  QImage qimg1, qimg2;

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
