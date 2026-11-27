#pragma once

#include "../datatypes.h"
#include "../core/camera_controller.h"
#include "../core/image_processing.h"

#include <QtCore/QThread>
#include <QTimer>
#include <QtGui/QImage>

#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdint>

class PipelineWorker : public QObject
{
    Q_OBJECT

public:
explicit PipelineWorker(QObject *parent = nullptr, ConfigData *params = {})
    : QObject(parent),
      m_cams(params->cam),
      m_hsvParams(params->hsv) {}

public slots:
    void startCapture();

signals:
    void newFrame(const Frame& frame1, const Frame& frame2);
    void finished();

private:
    void startProcessingWorkers();
    void stopProcessingWorkers();
    void processingWorker(int cameraIndex);
    void stopCapture();

    std::array<double, 2> m_thresholdMs{};
    std::array<double, 2> m_detectMs{};

    // Camera controller
    CameraController m_cams;

    // Each processing worker creates its own ImageProcessor
    HSVThreshParams m_hsvParams;

    // Persistent processing threads
    std::thread m_processingThread1;
    std::thread m_processingThread2;

    // Thread synchronization
    std::mutex m_processingMutex;
    std::condition_variable m_workCV;
    std::condition_variable m_doneCV;

    bool m_stopProcessing = false;

    uint64_t m_generation = 0;
    int m_completedWorkers = 0;

    // Frames shared between the main thread and processing workers
    std::array<Frame, 2> m_inputFrames;
    std::array<Frame, 2> m_processedFrames;
};
