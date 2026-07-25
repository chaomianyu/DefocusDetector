#pragma once
#include <opencv2/opencv.hpp>
#include "../config/FocusEvalConfig.h"

/**
 * 清晰度 / 眼睛开合度 质量指标计算
 */
struct SharpnessMetric {
    // ---- 配置 ----
    float clahe_clip   = 2.0f;
    int   clahe_grid   = 8;
    int   thresh_block = 11;
    int   thresh_C     = 2;

    void configure(const FocusEvalConfig& cfg) {
        clahe_clip   = cfg.clahe_clip;
        clahe_grid   = cfg.clahe_grid;
        thresh_block = cfg.thresh_block;
        thresh_C     = cfg.thresh_C;
    }

    // 拉普拉斯方差（模糊度量，越高越清晰）
    double compute_laplacian(const cv::Mat& roi) const;

    // 眼睛开合度（dark_ratio + 对比度融合，0=全闭 1=全开）
    double compute_eye_openness(const cv::Mat& eye_roi) const;
};
