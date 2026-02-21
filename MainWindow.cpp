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
    connect(cameras, &CameraController::newFrame, this, &MainWindow::updateCamera1Display);

    qDebug() << "Starting Cameras";

    cameras->startCamera(); 
}

MainWindow::~MainWindow() {

}

void MainWindow::updateCamera1Display(const QImage& image)
{
    ui->camera1->setPixmap(QPixmap::fromImage(image).scaled(ui->camera1->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
