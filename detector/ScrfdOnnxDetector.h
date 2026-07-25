#pragma once
#include "IFaceDetector.h"
#include <opencv2/dnn.hpp>
#include <unordered_map>

class ScrfdOnnxDetector : public IFaceDetector {
public:
    // 由 config 注入
    float conf_threshold  = 0.5f;
    float nms_threshold   = 0.4f;
    int   input_size      = 640;

    bool load(const std::string& model_path) override;
    std::vector<FaceLandmark> detect(const cv::Mat& image) override;
    bool is_ready() const override { return ready_; }

private:
    cv::dnn::Net net_;
    bool ready_ = false;

    // SCRFD 解码内部实现
    std::vector<FaceLandmark> decode_scrfd(
        const std::vector<cv::Mat>& outs,
        int img_w, int img_h) const;
};
