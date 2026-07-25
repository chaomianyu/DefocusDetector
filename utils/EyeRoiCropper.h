#pragma once
#include <opencv2/core/types.hpp>
#include <algorithm>

/**
 * 眼睛 ROI 裁剪
 * 从人脸关键点提取眼部区域，自动 clamp 到图像边界
 */
struct EyeRoiCropper {
    float roi_ratio  = 0.18f;
    int   min_size   = 24;

    cv::Rect crop(float eye_x, float eye_y, float face_width, int img_w, int img_h) const {
        int sz = std::max((int)(face_width * roi_ratio), min_size);
        int x  = (int)eye_x - sz / 2;
        int y  = (int)eye_y - sz / 2;
        int w  = sz;
        int h  = sz;

        // clamp
        x = std::max(0, x);
        y = std::max(0, y);
        w = std::min(w, img_w - x);
        h = std::min(h, img_h - y);
        return cv::Rect(x, y, std::max(0, w), std::max(0, h));
    }
};
