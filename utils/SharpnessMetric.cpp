#include "SharpnessMetric.h"
#include <opencv2/imgproc.hpp>

double SharpnessMetric::compute_laplacian(const cv::Mat& roi) const {
    if (roi.empty()) return 0;
    cv::Mat gray;
    if (roi.channels() == 3)
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    else
        gray = roi.clone();

    // 缩放到最大 256px 以加速计算
    if (gray.cols > 256 || gray.rows > 256) {
        double s = 256.0 / std::max(gray.cols, gray.rows);
        cv::resize(gray, gray, cv::Size(), s, s, cv::INTER_AREA);
    }

    cv::Mat lap;
    cv::Laplacian(gray, lap, CV_64F, 3);
    cv::Scalar m, sd;
    cv::meanStdDev(lap, m, sd);
    return sd[0] * sd[0];  // 方差
}

double SharpnessMetric::compute_eye_openness(const cv::Mat& eye_roi) const {
    if (eye_roi.empty()) return 0;
    cv::Mat gray;
    if (eye_roi.channels() == 3)
        cv::cvtColor(eye_roi, gray, cv::COLOR_BGR2GRAY);
    else
        gray = eye_roi.clone();

    static auto clahe = cv::createCLAHE(clahe_clip, cv::Size(clahe_grid, clahe_grid));
    cv::Mat enhanced;
    clahe->apply(gray, enhanced);

    cv::Mat binary;
    cv::adaptiveThreshold(enhanced, binary, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV, thresh_block, thresh_C);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);

    int nonzero = cv::countNonZero(binary);
    double total = binary.rows * binary.cols;
    double dark_ratio = nonzero / total;
    double openness = (dark_ratio - 0.03) / 0.25;
    openness = std::max(0.0, std::min(1.0, openness));

    // 垂直投影对比度
    cv::Mat v_proj;
    cv::reduce(gray, v_proj, 1, cv::REDUCE_AVG, CV_32F);
    double min_v, max_v;
    cv::minMaxLoc(v_proj, &min_v, &max_v);
    double contrast = (max_v - min_v) / 255.0;

    double combined = openness * 0.6 + contrast * 0.4;
    return std::max(0.0, std::min(1.0, combined));
}
