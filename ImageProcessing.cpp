#include "ImageProcessing.h"
#include "CameraParams.h"
#include <QtCore/QDebug>

using namespace cv;

ImageProcessor::ImageProcessor(QObject *parent) : QObject(parent),
                                                  m_greenMargin(10), m_lowThresh(160)
{
}

ImageProcessor::~ImageProcessor()
{
}

void ImageProcessor::processFrames(const Frame &frame1, const Frame &frame2, const Frame &frame3)
{
    Frame processed1;
    Frame processed2;
    Frame processed3;

    processed1.raw = frame1.raw.clone();
    processed2.raw = frame2.raw.clone();
    processed3.raw = frame3.raw.clone();

    processed1.thresh = tennisThreshold(processed1.raw);
    processed2.thresh = tennisThreshold(processed2.raw);
    processed3.thresh = tennisThreshold(processed3.raw);

    detectBall(processed1.raw, processed1.thresh, processed1.hasBall, processed1.ballCenter, processed1.ballRadius, processed1.annotated);
    detectBall(processed2.raw, processed2.thresh, processed2.hasBall, processed2.ballCenter, processed2.ballRadius, processed2.annotated);
    detectBall(processed3.raw, processed3.thresh, processed3.hasBall, processed3.ballCenter, processed3.ballRadius, processed3.annotated);
    emit newProcessedFrame(processed1, processed2, processed3);
}

cv::Mat ImageProcessor::tennisThreshold(cv::Mat raw)
{
    cv::Mat colour_channels[3];
    cv::Mat gauss;
    GaussianBlur(raw, gauss, Size(7, 7), 0);
    split(gauss, colour_channels);

    // Green must be dominant over blue and red
    cv::Mat greaterThanBlue, greaterThanRed, greenDominant;
    cv::compare(colour_channels[1], colour_channels[0] + m_greenMargin, greaterThanBlue, cv::CMP_GT);
    cv::compare(colour_channels[1], colour_channels[2] + m_greenMargin, greaterThanRed, cv::CMP_GT);
    cv::bitwise_and(greaterThanBlue, greaterThanRed, greenDominant);

    // Blue must be significantly lower than red and green
    cv::Mat lessThanRed, lessThanGreen, blueLow;
    cv::compare(colour_channels[0] + m_blueMargin, colour_channels[2], lessThanRed, cv::CMP_LT);
    cv::compare(colour_channels[0] + m_blueMargin, colour_channels[1], lessThanGreen, cv::CMP_LT);
    cv::bitwise_and(lessThanRed, lessThanGreen, blueLow);

    // Only keep pixels where both agree
    cv::Mat result;
    cv::bitwise_and(greenDominant, blueLow, result);
    return result;
}

cv::Mat ImageProcessor::hsvThreshold(cv::Mat raw)
{
    Mat hsv_image, hsv_img_gauss;
    cvtColor(raw, hsv_image, COLOR_BGR2HSV);
    GaussianBlur(hsv_image, hsv_img_gauss, cv::Size(5, 5), 0);

    Scalar lower_bound = Scalar(m_hueMin, m_saturationMin, m_valueMin);
    Scalar upper_bound = Scalar(m_hueMax, m_saturationMax, m_valueMax);

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
    if (bestArea < m_minSize)
        return cv::Point2f(-1, -1);
    if (contours[bestIdx].size() < 5)
        return cv::Point2f(-1, -1);

    cv::RotatedRect ellipse = cv::fitEllipse(contours[bestIdx]);
    float ratio = ellipse.size.width / ellipse.size.height;
    if (ratio > 1.0f)
        ratio = 1.0f / ratio;
    if (ratio < 0.60f)
        return cv::Point2f(-1, -1);

    cv::minEnclosingCircle(contours[bestIdx], center, radius);
    if (radius * 2 < m_minSize)
        return cv::Point2f(-1, -1);
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

void ImageProcessor::updateThreshParams(int margin, int lowThresh, int minSize, int margin_2)
{
    m_greenMargin = margin;
    m_lowThresh = lowThresh;
    m_minSize = minSize;
    m_blueMargin = margin_2;
}