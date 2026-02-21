#pragma once
#include <QMainWindow>
#include "CameraController.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void updateCameraDisplay(const QImage& image1, const QImage& image2);

private:
  Ui::MainWindow *ui;
  CameraController* cameras;
};
