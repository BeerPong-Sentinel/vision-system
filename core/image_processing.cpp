#include "image_processing.h"
#include <QtCore/QDebug>
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>

using namespace cv;

ImageProcessor::ImageProcessor() {
}

ImageProcessor::~ImageProcessor()
{
}

cv::Mat ImageProcessor::tennisThreshold(const cv::Mat& raw)
{
    Mat colour_channels[3];
    Mat gauss;
    GaussianBlur(raw, gauss, Size(7, 7), 0);
    Mat hsv;
    hsv = hsvThreshold(gauss);
    split(gauss, colour_channels);

    // Green must be dominant over blue and red
    Mat greaterThanBlue, greaterThanRed, greenDominant;
    compare(colour_channels[1], colour_channels[0] + m_greenMargin, greaterThanBlue, cv::CMP_GT);
    compare(colour_channels[1], colour_channels[2] + m_greenMargin, greaterThanRed, cv::CMP_GT);
    bitwise_and(greaterThanBlue, greaterThanRed, greenDominant);

    // Blue must be significantly lower than red and green
    Mat lessThanRed, lessThanGreen, blueLow;
    compare(colour_channels[0] + m_blueMargin, colour_channels[2], lessThanRed, cv::CMP_LT);
    compare(colour_channels[0] + m_blueMargin, colour_channels[1], lessThanGreen, cv::CMP_LT);
    bitwise_and(lessThanRed, lessThanGreen, blueLow);


    // Only keep pixels where both agree
    Mat result;
    bitwise_and(greenDominant, blueLow, result);
    bitwise_and(result, hsv, result);
    return result;
}

cv::Mat ImageProcessor::hsvThreshold(const cv::Mat& raw)
{
    Mat hsv_image, hsv_img_gauss;
    cvtColor(raw, hsv_image, COLOR_BGR2HSV);
    GaussianBlur(hsv_image, hsv_img_gauss, cv::Size(5, 5), 0);

    Scalar lower_bound = Scalar(m_hueMin, 40, 10);
    Scalar upper_bound = Scalar(m_hueMax, 255, 255);

    // Scalar lower_bound = Scalar(m_hueMin, m_saturationMin, m_valueMin);
    // Scalar upper_bound = Scalar(m_hueMax, m_saturationMax, m_valueMax);

    Mat masked;
    inRange(hsv_img_gauss, lower_bound, upper_bound, masked);
    return masked;
}

cv::Point2f ImageProcessor::detectBall(const cv::Mat& raw, const cv::Mat& thresh, bool &hasBall, cv::Point2f &center, float &radius, cv::Mat& annotated)
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
    /*
    if (ratio < 0.60f)
        return cv::Point2f(-1, -1);
    */
    cv::minEnclosingCircle(contours[bestIdx], center, radius);
    if (radius * 2 < m_minSize)
        return cv::Point2f(-1, -1);
    hasBall = true;
    cv::circle(annotated, center, radius, cv::Scalar(255, 0, 255), 2);
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


    std::cout << "m_hueMin = " << m_hueMin << std::endl;
    std::cout << "m_hueMax = " << m_hueMax << std::endl;
}

void ImageProcessor::updateThreshParams(int margin, int lowThresh, int minSize, int margin_2)
{
    m_greenMargin = margin;
    m_lowThresh = lowThresh;
    m_minSize = minSize;
    m_blueMargin = margin_2;
}
