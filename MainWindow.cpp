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
    cameras = new CameraController(this);
    connect(cameras, &CameraController::newFrame, this, &MainWindow::updateCameraDisplay);

    qDebug() << "Starting Cameras";

    cameras->startCamera(); 
}

MainWindow::~MainWindow() {

}

void MainWindow::updateCameraDisplay(const cv::Mat frame1, const cv::Mat frame2)
{
    QImage qimg1(frame1.data, frame1.cols, frame1.rows, static_cast<int>(frame1.step), QImage::Format_RGB888);
    QImage qimg2(frame2.data, frame2.cols, frame2.rows, static_cast<int>(frame2.step), QImage::Format_RGB888);

    ui->camera1->setPixmap(QPixmap::fromImage(qimg1).scaled(ui->camera1->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->camera2->setPixmap(QPixmap::fromImage(qimg2).scaled(ui->camera2->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
