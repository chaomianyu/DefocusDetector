#pragma once
#include <opencv2/core.hpp>
#include <vector>
#include "../model/FaceLandmark.h"

/**
 * 人脸检测器抽象接口
 * 可替换实现：YuNet / SCRFD / YOLO-Face
 */
class IFaceDetector {
public:
    virtual ~IFaceDetector() = default;

    // 返回是否加载成功
    virtual bool load(const std::string& model_path) = 0;

    // 执行检测，返回人脸列表（按置信度降序）
    virtual std::vector<FaceLandmark> detect(const cv::Mat& image) = 0;

    // 是否已就绪
    virtual bool is_ready() const = 0;
};
