#include "EyeFocusEvaluator.h"
#include "../utils/EyeRoiCropper.h"
#include "../utils/SharpnessMetric.h"

EyeFocusEvaluator::EyeFocusEvaluator(const FocusEvalConfig& cfg) : cfg_(cfg) {}

EyeRoiCropper EyeFocusEvaluator::cropper() const {
    EyeRoiCropper c;
    c.roi_ratio = cfg_.eye_roi_ratio;
    c.min_size  = cfg_.min_eye_size;
    return c;
}

SharpnessMetric EyeFocusEvaluator::metric() const {
    SharpnessMetric m;
    m.configure(cfg_);
    return m;
}

// ================================================================
// 一票否决入口
// ================================================================
FocusResult EyeFocusEvaluator::evaluate(const FaceLandmark& face,
                                        const cv::Mat& image) const {
    FocusResult r;
    int iw = image.cols, ih = image.rows;
    r.detection_conf = face.confidence;

    // 依次执行四项检查，任一失败即熔断
    if (!check_face_ratio(face, iw, ih, r)) return r;
    if (!check_eye_valid(face, iw, ih, r)) return r;
    if (!check_eye_blur(face, image, iw, ih, r)) return r;
    if (!check_eye_openness(face, image, iw, ih, r)) return r;

    r.passed = true;
    r.reason = VetoReason::PASS;
    return r;
}

// ================================================================
// 子规则
// ================================================================
bool EyeFocusEvaluator::check_face_ratio(const FaceLandmark& face,
                                          int iw, int ih,
                                          FocusResult& r) const {
    r.face_ratio = (double)face.width() * face.height() / (iw * ih);
    if (r.face_ratio < cfg_.min_face_ratio) {
        r.passed = false;
        r.reason = VetoReason::FACE_TOO_SMALL;
        return false;
    }
    if (r.face_ratio > cfg_.max_face_ratio) {
        r.passed = false;
        r.reason = VetoReason::FACE_TOO_LARGE;
        return false;
    }
    return true;
}

bool EyeFocusEvaluator::check_eye_valid(const FaceLandmark& face,
                                         int iw, int ih,
                                         FocusResult& r) const {
    auto valid = [iw, ih](float x, float y) {
        return x > 1 && x < iw - 1 && y > 1 && y < ih - 1;
    };
    if (!valid(face.le_x(), face.le_y()) || !valid(face.re_x(), face.re_y())) {
        r.passed = false;
        r.reason = VetoReason::EYES_NOT_DETECTED;
        return false;
    }
    return true;
}

bool EyeFocusEvaluator::check_eye_blur(const FaceLandmark& face,
                                        const cv::Mat& image,
                                        int iw, int ih,
                                        FocusResult& r) const {
    auto cr = cropper();
    cv::Rect lr = cr.crop(face.le_x(), face.le_y(), face.width(), iw, ih);
    cv::Rect rr = cr.crop(face.re_x(), face.re_y(), face.width(), iw, ih);
    if (lr.area() <= 0 || rr.area() <= 0) {
        r.passed = false;
        r.reason = VetoReason::EYES_NOT_DETECTED;
        return false;
    }

    auto sm = metric();
    r.left_eye_blur  = sm.compute_laplacian(image(lr));
    r.right_eye_blur = sm.compute_laplacian(image(rr));
    if (std::min(r.left_eye_blur, r.right_eye_blur) < cfg_.min_lap_var) {
        r.passed = false;
        r.reason = VetoReason::BLURRY_EYES;
        return false;
    }
    return true;
}

bool EyeFocusEvaluator::check_eye_openness(const FaceLandmark& face,
                                            const cv::Mat& image,
                                            int iw, int ih,
                                            FocusResult& r) const {
    auto cr = cropper();
    cv::Rect lr = cr.crop(face.le_x(), face.le_y(), face.width(), iw, ih);
    cv::Rect rr = cr.crop(face.re_x(), face.re_y(), face.width(), iw, ih);
    if (lr.area() <= 0 || rr.area() <= 0) {
        r.passed = false;
        r.reason = VetoReason::EYES_NOT_DETECTED;
        return false;
    }

    auto sm = metric();
    r.left_eye_open  = sm.compute_eye_openness(image(lr));
    r.right_eye_open = sm.compute_eye_openness(image(rr));
    if (std::min(r.left_eye_open, r.right_eye_open) < cfg_.min_eye_open) {
        r.passed = false;
        r.reason = VetoReason::EYES_CLOSED;
        return false;
    }
    return true;
}
