#pragma once
#include <opencv2/opencv.hpp>
#include "../model/FaceLandmark.h"

/**
 * 高分辨率图像下采样 → blobFromImage 归一化 → 坐标映射回原图
 */
struct ImagePreprocess {
    int    max_edge   = 2048;
    int    input_size = 640;

    // (img - 127.5) / 128.0
    double scale    = 1.0 / 128.0;
    double mean_val = 127.5 / 128.0;

    // 内部状态（由 prepare() 填充）
    cv::Mat detect_img;
    double  ds_factor = 1.0;  // 下采样比例
    int     orig_w    = 0;
    int     orig_h    = 0;

    // 返回 blob，同时填充 detect_img 和 ds_factor
    cv::Mat prepare(const cv::Mat& image) {
        orig_w = image.cols;
        orig_h = image.rows;
        ds_factor = 1.0;

        int long_edge = std::max(orig_w, orig_h);
        if (long_edge > max_edge) {
            ds_factor = (double)max_edge / long_edge;
            cv::resize(image, detect_img, cv::Size(), ds_factor, ds_factor, cv::INTER_AREA);
        } else {
            detect_img = image;
        }

        return cv::dnn::blobFromImage(
            detect_img, scale,
            cv::Size(input_size, input_size),
            cv::Scalar(mean_val, mean_val, mean_val),
            true, false, CV_32F);
    }

    // 将检测坐标映射回原图
    void map_to_original(FaceLandmark& f) const {
        if (ds_factor >= 1.0) return;
        float inv = (float)(1.0 / ds_factor);
        f.x1 *= inv; f.y1 *= inv; f.x2 *= inv; f.y2 *= inv;
        for (int i = 0; i < 10; i++) f.kps[i] *= inv;
    }
};
