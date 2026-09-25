#include "image_processing.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
#include <filesystem>
#include "cnpy.h"

using namespace cv;
namespace fs = std::filesystem;

namespace {
  cv::Mat npyDoubleToMat(cnpy::NpyArray arr) {
    size_t rows = arr.shape[0];
    size_t cols = arr.shape[1];
    
    return cv::Mat(rows, cols, CV_64FC1, arr.data<double>()).clone();
  }

  cv::Matx34d calculateProjMat(cv::Matx33d K, cv::Matx33d R, cv::Matx31d T) {
    cv::Matx34d RT;
    cv::hconcat(R, T, RT);

    return K * RT;
  }
}

ImageProcessor::ImageProcessor(std::string stereo_data_path) {
  cnpy::npz_t stereo_data = cnpy::npz_load(stereo_data_path);

  m_K_1 = npyDoubleToMat(stereo_data["K_1"]);
  m_K_2 = npyDoubleToMat(stereo_data["K_2"]);
  m_R = npyDoubleToMat(stereo_data["R"]);
  m_T = npyDoubleToMat(stereo_data["T"]);

  m_projMat1 = calculateProjMat(m_K_1, cv::Matx33d::eye(), cv::Matx31d::zeros());
  m_projMat2 = calculateProjMat(m_K_2, m_R, m_T);

  m_D_1 = npyDoubleToMat(stereo_data["D_1"]);
  m_D_2 = npyDoubleToMat(stereo_data["D_2"]);
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

cv::Point3f ImageProcessor::getBall3DCoords(cv::Point2f pos_1, cv::Point2f pos_2) {
    std::array<cv::Point2f, 1> undistorted_pt_1;
    std::array<cv::Point2f, 1> undistorted_pt_2;

    cv::undistortPoints(
      std::array<Point2f, 1>{pos_1},
      undistorted_pt_1,
      m_K_1,
      m_D_1,
      cv::noArray(),
      m_K_1 
    );

    cv::undistortPoints(
      std::array<Point2f, 1>{pos_2},
      undistorted_pt_2,
      m_K_2,
      m_D_2,
      cv::noArray(),
      m_K_2 
    );
    
    cv::Mat pos4D;
    
    cv::triangulatePoints(
      m_projMat1,
      m_projMat2,
      std::array<Point2d, 1>{undistorted_pt_1[0]},
      std::array<Point2d, 1>{undistorted_pt_2[0]},
      pos4D
    );
    
    cv::Point3f pos;
    
    double X = pos4D.at<double>(0, 0);
    double Y = pos4D.at<double>(1, 0);
    double Z = pos4D.at<double>(2, 0);
    double W = pos4D.at<double>(3, 0);
    
    if (W != 0.0) {
      pos.x = X/W;
      pos.y = Y/W;
      pos.z = Z/W;
    } else {
      // std::cout << "W = 0" << std::endl;
    }
    return pos;
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
