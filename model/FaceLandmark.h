#pragma once
#include <opencv2/core/types.hpp>

/**
 * 人脸检测结果
 * 包含边界框、置信度、5点关键点（双眼/鼻尖/嘴角左右）
 */
struct FaceLandmark {
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    float confidence = 0;

    // 关键点：左眼→右眼→鼻尖→左嘴角→右嘴角
    float kps[10]{};   // [x0,y0, x1,y1, x2,y2, x3,y3, x4,y4]

    // 便捷访问
    float le_x() const { return kps[0]; }
    float le_y() const { return kps[1]; }
    float re_x() const { return kps[2]; }
    float re_y() const { return kps[3]; }

    float width()  const { return x2 - x1; }
    float height() const { return y2 - y1; }

    cv::Rect rect() const {
        return cv::Rect(
            cv::Point((int)x1, (int)y1),
            cv::Point((int)x2, (int)y2));
    }

    // 空检测（sentinel）
    static FaceLandmark null() { FaceLandmark f; f.confidence = -1.f; return f; }
    bool is_null() const { return confidence < 0.f; }
};
