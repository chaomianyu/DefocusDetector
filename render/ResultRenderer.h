#pragma once
#include <opencv2/imgproc.hpp>
#include "../model/FaceLandmark.h"
#include "../model/FocusResult.h"

/**
 * 结果可视化渲染
 */
struct ResultRenderer {
    cv::Scalar pass_color = cv::Scalar(0, 255, 0);
    cv::Scalar fail_color = cv::Scalar(0, 0, 255);
    cv::Scalar kp_color   = cv::Scalar(255, 0, 0);

    void draw(cv::Mat& image, const FaceLandmark& face,
              const FocusResult& r) const {
        auto color = r.passed ? pass_color : fail_color;

        // 人脸框
        cv::rectangle(image, face.rect(), color, 3);

        // 双眼关键点
        cv::circle(image, cv::Point((int)face.le_x(), (int)face.le_y()),
                   6, kp_color, -1);
        cv::circle(image, cv::Point((int)face.re_x(), (int)face.re_y()),
                   6, kp_color, -1);

        // 判定文字
        std::string tag = r.passed ? "PASS" : "FAIL - " +
                          std::string(veto_reason_str(r.reason));
        cv::putText(image, tag, cv::Point(30, 60),
                    cv::FONT_HERSHEY_SIMPLEX, 1.2, color, 3);
    }
};
