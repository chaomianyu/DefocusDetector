#pragma once
#include <string>
#include <vector>

/**
 * 虚焦评估配置
 * 所有阈值/参数集中管理，调参无需改动业务代码
 */
struct FocusEvalConfig {
    // ---- 检测器 ----
    float  conf_threshold   = 0.5f;
    float  nms_threshold    = 0.4f;
    int    input_size       = 640;
    int    max_detect_edge  = 2048;     // 高分辨率下采样阈值

    // ---- 一票否决 ----
    float  min_face_ratio   = 0.001f;   // 0.1%
    float  max_face_ratio   = 0.85f;    // 85%
    float  min_lap_var      = 80.0f;    // 拉普拉斯方差下限
    float  min_eye_open     = 0.18f;    // 眼睛开合度下限

    // ---- 眼睛 ROI ----
    float  eye_roi_ratio    = 0.18f;    // ROI 相对人脸宽度
    int    min_eye_size     = 24;       // ROI 最小尺寸

    // ---- 预处理 ----
    float  clahe_clip       = 2.0f;
    int    clahe_grid       = 8;
    int    thresh_block     = 11;
    int    thresh_C         = 2;

    // ---- 路径 ----
    std::string input_dir   = "after";
    std::string output_dir  = "detection_result";
    bool        save_output = false;

    std::vector<std::string> model_paths = {
        "models/scrfd_2.5g.onnx",
        "models/scrfd_2.5g_kps.onnx"
    };
};
