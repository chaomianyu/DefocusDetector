#include "FocusApi.h"
#include "../detector/ScrfdOnnxDetector.h"
#include "../core/EyeFocusEvaluator.h"
#include "../render/ResultRenderer.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <opencv2/imgcodecs.hpp>

namespace fs = std::filesystem;
using Clock = std::chrono::high_resolution_clock;

// ===================================================================
FocusApi::FocusApi() {}

bool FocusApi::parse_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--help") { show_help_ = true; return true; }
        if (a == "--input"  || a == "-i") {
            if (i + 1 < argc) cfg_.input_dir = argv[++i];
        }
        else if (a == "--output" || a == "-o") {
            if (i + 1 < argc) cfg_.output_dir = argv[++i];
        }
        else if (a == "--save" || a == "-s") {
            cfg_.save_output = true;
        }
    }
    return true;
}

void FocusApi::print_usage() const {
    std::cout <<
        "Usage: focus_evaluate [options]\n\n"
        "Options:\n"
        "  --input <dir>, -i    Input directory (default: after)\n"
        "  --output <dir>, -o   Output directory (default: detection_result)\n"
        "  --save, -s           Save annotated images\n"
        "  --help               Show help\n";
}

void FocusApi::print_header() const {
    std::cout << "\n================================\n";
    std::cout << " Focus Evaluate\n";
    std::cout << "================================\n";
    std::cout << "Input:       " << cfg_.input_dir << "\n";
    std::cout << "Output:      " << cfg_.output_dir << "\n";
    std::cout << "Save output: " << (cfg_.save_output ? "Yes" : "No") << "\n\n";
}

// ===================================================================
int FocusApi::run() {
    if (show_help_) { print_usage(); return 0; }

    std::ofstream log("focus_evaluate.log");
    if (!log.is_open())
        std::cerr << "WARNING: Cannot create log file" << std::endl;

    log << "Focus Evaluate\n";
    fs::create_directories(cfg_.output_dir);

    // ---------- 加载检测器 ----------
    auto detector = std::make_unique<ScrfdOnnxDetector>();
    detector->conf_threshold = cfg_.conf_threshold;
    detector->nms_threshold  = cfg_.nms_threshold;
    detector->input_size     = cfg_.input_size;

    bool loaded = false;
    for (auto& mp : cfg_.model_paths) {
        if (!fs::exists(mp)) continue;
        if (detector->load(mp)) {
            loaded = true;
            std::cout << "Loaded: " << mp << std::endl;
            log << "Model: " << mp << std::endl;
            break;
        }
    }
    if (!loaded) {
        std::cerr << "ERROR: No model found" << std::endl;
        return 1;
    }

    // ---------- 创建评估器 ----------
    EyeFocusEvaluator evaluator(cfg_);
    ResultRenderer renderer;

    print_header();

    // ---------- 遍历处理 ----------
    int total = 0;
    auto t_all = Clock::now();
    for (const auto& e : fs::directory_iterator(cfg_.input_dir)) {
        if (!e.is_regular_file()) continue;
        std::string fn = e.path().filename().string();
        auto dot = fn.rfind('.');
        if (dot == std::string::npos) continue;
        std::string ext = fn.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != "bmp"  && ext != "jpg"  && ext != "jpeg" &&
            ext != "png"  && ext != "webp") continue;

        std::cout << "[PROCESS] " << fn << std::endl;
        log << "\n--- " << fn << " ---" << std::endl;
        total++;

        // 读取
        auto t0 = Clock::now();
        cv::Mat img = cv::imread(e.path().string());
        double read_ms = std::chrono::duration<double, std::milli>(
            Clock::now() - t0).count();
        if (img.empty()) {
            std::cerr << "  Error: read failed" << std::endl;
            continue;
        }
        std::cout << "  Image: " << img.cols << "x" << img.rows
                  << " (read: " << std::fixed << std::setprecision(1)
                  << read_ms << "ms)" << std::endl;
        log << "Image: " << img.cols << "x" << img.rows
            << " read=" << read_ms << "ms" << std::endl;

        // 检测
        t0 = Clock::now();
        auto dets = detector->detect(img);
        double det_ms = std::chrono::duration<double, std::milli>(
            Clock::now() - t0).count();

        if (dets.empty()) {
            std::cout << "  Result: No face ("
                      << std::fixed << std::setprecision(1) << det_ms << "ms)"
                      << std::endl;
            log << "Result: No face" << std::endl;
            continue;
        }

        // 评估最佳检测框
        const auto& best = dets[0];
        FocusResult r = evaluator.evaluate(best, img);

        std::cout << "  Detect: " << std::fixed << std::setprecision(1)
                  << det_ms << "ms  conf=" << best.confidence
                  << "  faces=" << dets.size() << std::endl;
        std::cout << "  " << r.to_string() << std::endl;

        log << "Detect: " << det_ms << "ms conf=" << best.confidence << std::endl;
        log << r.to_string() << std::endl;

        // 保存标注图
        if (cfg_.save_output) {
            renderer.draw(img, best, r);
            cv::imwrite(cfg_.output_dir + "/" + fn, img);
        }

        std::cout << std::endl;
    }

    auto elapsed = std::chrono::duration<double, std::milli>(
        Clock::now() - t_all).count();
    std::cout << "================================\n";
    std::cout << " Complete\n";
    std::cout << "================================\n";
    std::cout << "Total:  " << total << " files\n";
    std::cout << "Time:   " << std::fixed << std::setprecision(1)
              << elapsed << " ms\n";
    if (total > 0)
        std::cout << "Avg:    " << elapsed / total << " ms/file\n";

    log << "\nTotal=" << total << " time=" << elapsed << "ms" << std::endl;
    return 0;
}
