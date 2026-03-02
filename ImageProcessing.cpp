#include "ImageProcessing.h"
#include "CameraParams.h"
#include <QtCore/QDebug>

using namespace cv;

ImageProcessor::ImageProcessor(QObject *parent) : QObject(parent)
{
}

ImageProcessor::~ImageProcessor()
{
}

void ImageProcessor::processFrames(const Frame &frame1, const Frame &frame2, const Frame &frame3)
{
    // Copy Frames to Modify
    Frame processed1;
    Frame processed2;
    Frame processed3;

    processed1.raw = frame1.raw.clone();
    processed2.raw = frame2.raw.clone();
    processed3.raw = frame3.raw.clone();

    Mat thresh1, thresh2, thres3;

    processed1.thresh = greenThreshold(processed1.raw);
    processed2.thresh = greenThreshold(processed2.raw);
    processed3.thresh = greenThreshold(processed3.raw);

    // processed1.thresh = hsvThreshold(processed1.raw);
    // processed2.thresh = hsvThreshold(processed2.raw);
    // processed3.thresh = hsvThreshold(processed3.raw);
    detectBall(processed1.raw, processed1.thresh, processed1.hasBall, processed1.ballCenter, processed1.ballRadius, processed1.annotated);
    detectBall(processed2.raw, processed2.thresh, processed2.hasBall, processed2.ballCenter, processed2.ballRadius, processed2.annotated);
    detectBall(processed3.raw, processed3.thresh, processed3.hasBall, processed3.ballCenter, processed3.ballRadius, processed3.annotated);
    emit newProcessedFrame(processed1, processed2, processed3);
}

cv::Mat ImageProcessor::greenThreshold(cv::Mat raw)
{
    cv::Mat colour_channels[3];
    Mat green_image_gauss;
    GaussianBlur(raw, green_image_gauss, Size(7, 7), 0);

    cv::split(raw, colour_channels);

    // Green must be dominant over blue and red
    cv::Mat greaterThanBlue, greaterThanRed, greenDominant;
    int margin = 10;
    cv::compare(colour_channels[1], colour_channels[0] + margin, greaterThanBlue, cv::CMP_GT);
    cv::compare(colour_channels[1], colour_channels[2] + margin, greaterThanRed, cv::CMP_GT);
    cv::bitwise_and(greaterThanBlue, greaterThanRed, greenDominant);

    // Also threshold the green channel for brightness
    cv::Mat thresh;
    cv::threshold(colour_channels[1], thresh, 157, 255, 0);

    // Only keep pixels that are both bright green AND dominant
    cv::Mat result;
    cv::bitwise_and(thresh, greenDominant, result);
    return greenDominant;
}

cv::Mat ImageProcessor::hsvThreshold(cv::Mat raw)
{

    Mat hsv_image, hsv_img_gauss;
    cvtColor(raw, hsv_image, COLOR_BGR2HSV);
    GaussianBlur(hsv_image, hsv_img_gauss, cv::Size(5, 5), 0);
    // Scalar lower_bound = Scalar(m_hueMin, m_saturationMin, m_valueMin);
    // Scalar upper_bound = Scalar(m_hueMax, m_saturationMax, m_valueMax);

    Scalar lower_bound = Scalar(20, 40, 10);
    Scalar upper_bound = Scalar(100, 255, 255);

    Mat masked;
    inRange(hsv_img_gauss, lower_bound, upper_bound, masked);

    return masked;
}

cv::Point2f ImageProcessor::detectBall(cv::Mat raw, cv::Mat thresh, bool &hasBall, cv::Point2f &center, float &radius, cv::Mat &annotated)
{
    hasBall = false;
    annotated = raw.clone();
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty())
        return cv::Point2f(-1, -1);

    int bestIdx = 0;
    double bestArea = 0;
    for (int i = 0; i < contours.size(); i++)
    {
        double area = cv::contourArea(contours[i]);
        if (area > bestArea)
        {
            bestArea = area;
            bestIdx = i;
        }
    }
    if (bestArea < 200)
        return cv::Point2f(-1, -1);
    if (contours[bestIdx].size() < 5)
        return cv::Point2f(-1, -1);

    cv::RotatedRect ellipse = cv::fitEllipse(contours[bestIdx]);
    float ratio = ellipse.size.width / ellipse.size.height;
    if (ratio > 1.0f)
        ratio = 1.0f / ratio;
    if (ratio < 0.75f)
        return cv::Point2f(-1, -1);

    cv::minEnclosingCircle(contours[bestIdx], center, radius);
    hasBall = true;
    cv::circle(annotated, center, radius, cv::Scalar(0, 255, 0), 2);
    return center;
}

void ImageProcessor::updateHSVParams(const int h_min, const int h_max, const int s_min, const int s_max, const int v_min, const int v_max)
{
    m_hueMin = h_min;
    m_hueMax = h_max;
    m_saturationMin = s_min;
    m_saturationMax = s_max;
    m_valueMin = v_min;
    m_valueMax = v_max;
}
