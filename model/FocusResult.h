#pragma once
#include <string>

/**
 * 一票否决判定原因
 */
enum class VetoReason {
    PASS = 0,
    NO_FACE_DETECTED,
    FACE_TOO_SMALL,
    FACE_TOO_LARGE,
    EYES_NOT_DETECTED,
    BLURRY_EYES,
    EYES_CLOSED,
    INTERNAL_ERROR
};

inline const char* veto_reason_str(VetoReason r) {
    switch (r) {
    case VetoReason::PASS:              return "PASS";
    case VetoReason::NO_FACE_DETECTED:  return "NO_FACE_DETECTED";
    case VetoReason::FACE_TOO_SMALL:    return "FACE_TOO_SMALL";
    case VetoReason::FACE_TOO_LARGE:    return "FACE_TOO_LARGE";
    case VetoReason::EYES_NOT_DETECTED: return "EYES_NOT_DETECTED";
    case VetoReason::BLURRY_EYES:       return "BLURRY_EYES";
    case VetoReason::EYES_CLOSED:       return "EYES_CLOSED";
    case VetoReason::INTERNAL_ERROR:    return "INTERNAL_ERROR";
    }
    return "UNKNOWN";
}

/**
 * 单张照片的虚焦评估结果
 */
struct FocusResult {
    bool        passed = false;
    VetoReason  reason = VetoReason::PASS;

    double face_ratio     = 0;
    double left_eye_blur  = 0;
    double right_eye_blur = 0;
    double left_eye_open  = 0;
    double right_eye_open = 0;

    float  detection_conf = 0;

    // 格式化输出
    std::string to_string() const {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "ratio=%.1f%%  lap=%d/%d  open=%.2f/%.2f  conf=%.2f  => %s",
            face_ratio * 100.0,
            (int)left_eye_blur, (int)right_eye_blur,
            left_eye_open, right_eye_open,
            detection_conf,
            veto_reason_str(reason));
        return std::string(buf);
    }
};
