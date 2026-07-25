#pragma once
#include <opencv2/core.hpp>
#include "../model/FaceLandmark.h"
#include "../model/FocusResult.h"
#include "../config/FocusEvalConfig.h"

/**
 * 眼睛虚焦评估器
 * 依赖注入 IFaceDetector，不绑定具体检测器实现
 */
class EyeFocusEvaluator {
public:
    explicit EyeFocusEvaluator(const FocusEvalConfig& cfg);

    // 对单个检测结果执行一票否决
    FocusResult evaluate(const FaceLandmark& face, const cv::Mat& image) const;

    // 判定是否通过
    bool is_pass(const FocusResult& r) const { return r.passed; }

private:
    FocusEvalConfig cfg_;

    // 工具类组合
    struct EyeRoiCropper cropper() const;
    struct SharpnessMetric metric() const;

    // 一票否决子规则
    bool check_face_ratio(const FaceLandmark& face, int iw, int ih,
                          FocusResult& r) const;
    bool check_eye_valid(const FaceLandmark& face, int iw, int ih,
                         FocusResult& r) const;
    bool check_eye_blur(const FaceLandmark& face, const cv::Mat& image,
                        int iw, int ih, FocusResult& r) const;
    bool check_eye_openness(const FaceLandmark& face, const cv::Mat& image,
                            int iw, int ih, FocusResult& r) const;
};
