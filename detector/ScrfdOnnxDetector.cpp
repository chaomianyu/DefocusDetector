#include "ScrfdOnnxDetector.h"
#include "../utils/ImagePreprocess.h"
#include <algorithm>
#include <cmath>

// ===================================================================
// IoU / NMS
// ===================================================================
namespace {
float detection_iou(const FaceLandmark& a, const FaceLandmark& b) {
    float x1 = std::max(a.x1, b.x1), y1 = std::max(a.y1, b.y1);
    float x2 = std::min(a.x2, b.x2), y2 = std::min(a.y2, b.y2);
    float inter = std::max(0.f, x2 - x1) * std::max(0.f, y2 - y1);
    float area_a = (a.x2 - a.x1) * (a.y2 - a.y1);
    float area_b = (b.x2 - b.x1) * (b.y2 - b.y1);
    float uni = area_a + area_b - inter;
    return uni > 0 ? inter / uni : 0;
}

std::vector<FaceLandmark> apply_nms(std::vector<FaceLandmark>& dets, float thresh) {
    std::sort(dets.begin(), dets.end(),
              [](const FaceLandmark& a, const FaceLandmark& b) {
                  return a.confidence > b.confidence;
              });
    std::vector<bool> suppressed(dets.size(), false);
    std::vector<FaceLandmark> keep;
    for (size_t i = 0; i < dets.size(); i++) {
        if (suppressed[i]) continue;
        keep.push_back(dets[i]);
        for (size_t j = i + 1; j < dets.size(); j++) {
            if (suppressed[j]) continue;
            if (detection_iou(dets[i], dets[j]) > thresh) suppressed[j] = true;
        }
    }
    return keep;
}

// distance2bbox / distance2kps
void distance2bbox(const float* points, const float* distance,
                   int n, float* bboxes) {
    for (int i = 0; i < n; i++) {
        float cx = points[i * 2],     cy = points[i * 2 + 1];
        float dl = distance[i * 4],   dt = distance[i * 4 + 1];
        float dr = distance[i * 4 + 2], db = distance[i * 4 + 3];
        bboxes[i * 4]     = cx - dl;
        bboxes[i * 4 + 1] = cy - dt;
        bboxes[i * 4 + 2] = cx + dr;
        bboxes[i * 4 + 3] = cy + db;
    }
}

void distance2kps(const float* points, const float* distance,
                  int n, float* kpss) {
    for (int i = 0; i < n; i++) {
        float cx = points[i * 2], cy = points[i * 2 + 1];
        for (int k = 0; k < 5; k++) {
            kpss[i * 10 + k * 2]     = cx + distance[i * 10 + k * 2];
            kpss[i * 10 + k * 2 + 1] = cy + distance[i * 10 + k * 2 + 1];
        }
    }
}

const std::vector<float>& get_cached_anchors(int h, int w, int stride) {
    static std::unordered_map<int, std::vector<float>> cache;
    int key = (h << 20) | (w << 8) | stride;
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;

    constexpr int NUM_ANCHORS = 2;
    std::vector<float> anchors(h * w * NUM_ANCHORS * 2);
    int idx = 0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float cx = (float)(x * stride);
            float cy = (float)(y * stride);
            for (int a = 0; a < NUM_ANCHORS; a++) {
                anchors[idx++] = cx;
                anchors[idx++] = cy;
            }
        }
    }
    cache[key] = std::move(anchors);
    return cache[key];
}

} // namespace

// ===================================================================
// public API
// ===================================================================
bool ScrfdOnnxDetector::load(const std::string& model_path) {
    try {
        net_ = cv::dnn::readNet(model_path);
        ready_ = !net_.empty();
    } catch (const cv::Exception&) {
        ready_ = false;
    }
    return ready_;
}

std::vector<FaceLandmark> ScrfdOnnxDetector::detect(const cv::Mat& image) {
    if (!ready_ || image.empty()) return {};

    ImagePreprocess prep;
    prep.max_edge   = 2048;
    prep.input_size = input_size;

    cv::Mat blob = prep.prepare(image);

    net_.setInput(blob);
    std::vector<cv::String> out_names = net_.getUnconnectedOutLayersNames();
    std::vector<cv::Mat> outs;
    net_.forward(outs, out_names);

    if (outs.size() < 9) return {};

    auto dets = decode_scrfd(outs, prep.detect_img.cols, prep.detect_img.rows);

    // 映射回原图坐标
    for (auto& d : dets) prep.map_to_original(d);

    return dets;
}

// ===================================================================
// SCRFD 解码
// ===================================================================
std::vector<FaceLandmark> ScrfdOnnxDetector::decode_scrfd(
    const std::vector<cv::Mat>& outs, int img_w, int img_h) const
{
    std::vector<FaceLandmark> dets;
    constexpr int STRIDES[3]  = {8, 16, 32};
    constexpr int NUM_ANCHORS = 2;

    float scale_x = (float)img_w / input_size;
    float scale_y = (float)img_h / input_size;

    for (int s = 0; s < 3; s++) {
        int stride = STRIDES[s];
        int h = input_size / stride;
        int w = input_size / stride;

        const cv::Mat& score_mat = outs[s];
        const cv::Mat& bbox_mat  = outs[s + 3];
        const cv::Mat& kps_mat   = outs[s + 6];

        int total = h * w * NUM_ANCHORS;
        const auto& anchors = get_cached_anchors(h, w, stride);

        const float* scores = score_mat.ptr<float>();
        const float* bbox_r = bbox_mat.ptr<float>();
        const float* kps_r  = kps_mat.ptr<float>();

        // 过滤
        std::vector<int> pos;
        pos.reserve(total);
        for (int i = 0; i < total; i++)
            if (scores[i] >= conf_threshold) pos.push_back(i);
        if (pos.empty()) continue;

        int n = (int)pos.size();

        // 提取 && 预乘 stride
        std::vector<float> bbox_scaled(n * 4);
        std::vector<float> kps_scaled(n * 10);
        std::vector<float> sel_anchors(n * 2);
        for (int i = 0; i < n; i++) {
            int idx = pos[i];
            for (int j = 0; j < 4;  j++) bbox_scaled[i * 4  + j] = bbox_r[idx * 4  + j] * stride;
            for (int j = 0; j < 10; j++) kps_scaled[i * 10 + j] = kps_r [idx * 10 + j] * stride;
            sel_anchors[i * 2]     = anchors[idx * 2];
            sel_anchors[i * 2 + 1] = anchors[idx * 2 + 1];
        }

        std::vector<float> bboxes(n * 4), kpss(n * 10);
        distance2bbox(sel_anchors.data(), bbox_scaled.data(), n, bboxes.data());
        distance2kps(sel_anchors.data(), kps_scaled.data(), n, kpss.data());

        for (int i = 0; i < n; i++) {
            FaceLandmark f;
            f.x1   = bboxes[i * 4]     * scale_x;
            f.y1   = bboxes[i * 4 + 1] * scale_y;
            f.x2   = bboxes[i * 4 + 2] * scale_x;
            f.y2   = bboxes[i * 4 + 3] * scale_y;
            f.confidence = scores[pos[i]];
            for (int k = 0; k < 10; k++)
                f.kps[k] = kpss[i * 10 + k] * (k % 2 == 0 ? scale_x : scale_y);
            dets.push_back(f);
        }
    }
    return apply_nms(dets, nms_threshold);
}
