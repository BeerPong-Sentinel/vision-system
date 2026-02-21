#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QtCore/QDebug>

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

void MainWindow::updateCameraDisplay(const QImage& image1, const QImage& image2)
{
    ui->camera1->setPixmap(QPixmap::fromImage(image1).scaled(ui->camera1->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->camera2->setPixmap(QPixmap::fromImage(image2).scaled(ui->camera2->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
