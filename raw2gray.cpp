/**
 * ============================================================
 * JPG / RAW 转无损 WebP 批量转换工具
 * 依赖：OpenCV 5.0, LibRaw 0.21.x
 *
 * 工作流程：
 *   before/ 中的 JPG 或 RAW 文件 → 转换 → after/ 中的无损 WebP
 * ============================================================
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#include "libraw/libraw.h"

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// 图像格式转换辅助
// ---------------------------------------------------------------------------

// 将任意通道数的 Mat 统一转换为 BGR 三通道格式
cv::Mat bgr_to_webp_format(const cv::Mat& input) {
    if (input.channels() == 3) {
        return input.clone();
    } else if (input.channels() == 4) {
        cv::Mat bgr;
        cv::cvtColor(input, bgr, cv::COLOR_BGRA2BGR);
        return bgr;
    }
    return input.clone();
}

// 以无损 WebP 格式保存图像（保留原始色彩）
bool save_as_webp(const cv::Mat& image, const std::string& output_path) {
    cv::Mat bgr = bgr_to_webp_format(image);

    std::vector<int> params;
    params.push_back(cv::IMWRITE_WEBP_LOSSLESS_MODE);
    params.push_back(cv::IMWRITE_WEBP_LOSSLESS_PRESERVE_COLOR);

    bool success = cv::imwrite(output_path, bgr, params);
    if (!success) {
        std::cerr << "  Cannot write: " << output_path << std::endl;
    }
    return success;
}

// ---------------------------------------------------------------------------
// JPG 转换
// ---------------------------------------------------------------------------

// JPG 转无损 WebP：直接 OpenCV imread + 无损写
bool jpg_to_webp(const std::string& input_path, const std::string& output_path) {
    cv::Mat image = cv::imread(input_path, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Cannot read image: " << input_path << std::endl;
        return false;
    }

    std::cout << "  Image: " << image.cols << "x" << image.rows << ", " 
              << image.channels() << " channels" << std::endl;

    return save_as_webp(image, output_path);
}

// ---------------------------------------------------------------------------
// RAW 转换（核心：LibRaw 解码）
// ---------------------------------------------------------------------------

// RAW 转无损 WebP：LibRaw open → unpack → dcraw_process → make_mem_image → OpenCV Mat → WebP
bool raw_to_webp(const std::string& input_path, const std::string& output_path) {
    LibRaw raw_processor;

    // 步骤 1：打开 RAW 文件
    int ret = raw_processor.open_file(input_path.c_str());
    if (ret != LIBRAW_SUCCESS) {
        std::cerr << "Cannot open RAW file: " << input_path << " (error: " << ret << ")" << std::endl;
        return false;
    }

    // 步骤 2：解包 RAW 数据
    ret = raw_processor.unpack();
    if (ret != LIBRAW_SUCCESS) {
        std::cerr << "Cannot unpack RAW file: " << input_path << " (error: " << ret << ")" << std::endl;
        raw_processor.recycle();
        return false;
    }

    // 步骤 3：设置输出参数（8-bit 高质量输出）
    raw_processor.imgdata.params.output_tiff = 0;
    raw_processor.imgdata.params.user_qual = 3;
    raw_processor.imgdata.params.output_bps = 8;

    // 步骤 4：dcraw 后期处理（去马赛克、白平衡等）
    ret = raw_processor.dcraw_process();
    if (ret != LIBRAW_SUCCESS) {
        std::cerr << "RAW processing failed: " << input_path << " (error: " << ret << ")" << std::endl;
        raw_processor.recycle();
        return false;
    }

    // 步骤 5：从 LibRaw 获取处理后的像素数据
    int errcode = 0;
    libraw_processed_image_t* image = raw_processor.dcraw_make_mem_image(&errcode);
    if (!image || errcode != LIBRAW_SUCCESS) {
        std::cerr << "Cannot get processed image data: " << input_path << std::endl;
        raw_processor.recycle();
        return false;
    }

    int width = image->width;
    int height = image->height;
    int channels = image->colors;

    std::cout << "  Image: " << width << "x" << height << ", " << channels << " channels" << std::endl;

    // 步骤 6：LibRaw 数据 → OpenCV Mat（RAW 数据为 RGB/RGBA，需转 BGR）
    int cv_type = (channels == 4) ? CV_8UC4 : CV_8UC3;
    cv::Mat color_image(height, width, cv_type, image->data);

    cv::Mat bgr_image;
    if (channels == 4) {
        cv::cvtColor(color_image, bgr_image, cv::COLOR_RGBA2BGR);
    } else if (channels == 3) {
        cv::cvtColor(color_image, bgr_image, cv::COLOR_RGB2BGR);
    } else {
        bgr_image = color_image.clone();
    }

    // 步骤 7：保存为无损 WebP
    bool success = save_as_webp(bgr_image, output_path);

    // 步骤 8：释放 LibRaw 内存
    LibRaw::dcraw_clear_mem(image);
    raw_processor.recycle();

    return success;
}

// ---------------------------------------------------------------------------
// 文件名工具函数
// ---------------------------------------------------------------------------

// 获取不含扩展名的文件名（stem）
std::string get_stem(const std::string& path)
{
    fs::path p(path);
    return p.stem().string();
}

// 获取小写扩展名
std::string get_extension_lower(const std::string& path)
{
    fs::path p(path);
    std::string ext = p.extension().string();
    for (auto& c : ext) c = tolower(c);
    return ext;
}

// ===================================================================
// 主函数：遍历 before/ → 转换 → after/
// ===================================================================
int main(int argc, char* argv[]) {
    std::ofstream logfile("raw2gray.log");
    logfile << "Program started\n";

    std::string input_dir = "before";
    std::string output_dir = "after";

    logfile << "Input dir: " << input_dir << std::endl;
    logfile << "Output dir: " << output_dir << std::endl;
    logfile << "Output dir exists: " << fs::exists(output_dir) << std::endl;

    if (!fs::exists(output_dir)) {
        fs::create_directory(output_dir);
    }

    // LibRaw 支持的全部 RAW 扩展名（40+ 种相机格式）
    std::vector<std::string> raw_extensions = {
        ".raw", ".rw2", ".cr2", ".nef", ".arw", ".dng",
        ".orf", ".raf", ".pef", ".srw", ".x3f", ".iiq",
        ".kdc", ".dcr", ".mrw", ".ptx", ".cap", ".eip",
        ".3fr", ".mos", ".mef", ".nrw", ".rwl", ".sr2",
        ".srf", ".erf", ".bay", ".cs1", ".fff", ".gry",
        ".k25", ".mdc", ".mfw", ".nkm", ".nx2", ".pnl",
        ".pxn", ".rdc", ".r3d", ".rdb", ".scn", ".xrf"
    };

    std::vector<std::string> jpg_extensions = {
        ".jpg", ".jpeg", ".jpe", ".jfif", ".jfi"
    };

    std::cout << "================================\n";
    std::cout << " Image to WebP Converter\n";
    std::cout << " (JPG / RAW to Lossless WebP)\n";
    std::cout << "================================\n\n";
    std::cout << "Input dir: " << input_dir << std::endl;
    std::cout << "Output dir: " << output_dir << std::endl << std::endl;

    int total = 0;
    int success_count = 0;
    int fail_count = 0;
    int jpg_count = 0;
    int raw_count = 0;

    logfile << "Starting directory iteration\n";
    try {
        // 遍历输入目录
        for (const auto& entry : fs::directory_iterator(input_dir)) {
            if (!entry.is_regular_file()) continue;

            std::string filename = entry.path().filename().string();
            std::string ext = get_extension_lower(filename);
            logfile << "Found file: " << filename << ", ext: " << ext << std::endl;

            // 判断文件类型
            bool is_raw = false;
            bool is_jpg = false;

            for (const auto& raw_ext : raw_extensions) {
                if (ext == raw_ext) {
                    is_raw = true;
                    break;
                }
            }

            if (!is_raw) {
                for (const auto& jpg_ext : jpg_extensions) {
                    if (ext == jpg_ext) {
                        is_jpg = true;
                        break;
                    }
                }
            }

            if (!is_raw && !is_jpg) {
                std::cout << "[SKIP] " << filename << " (not JPG or RAW format)" << std::endl;
                logfile << "Skipping: " << filename << std::endl;
                continue;
            }

            std::string input_path = entry.path().string();
            std::string stem = get_stem(filename);
            std::string output_path = output_dir + "/" + stem + ".webp";

            std::cout << "[PROCESS] " << filename << " -> " << stem << ".webp";
            if (is_jpg) {
                std::cout << " (JPG)";
                jpg_count++;
            } else {
                std::cout << " (RAW)";
                raw_count++;
            }
            std::cout << std::endl;
            logfile << "Processing: " << input_path << " -> " << output_path << std::endl;

            // 根据类型选择转换函数
            total++;
            bool success = false;
            if (is_jpg) {
                success = jpg_to_webp(input_path, output_path);
            } else {
                success = raw_to_webp(input_path, output_path);
            }

            if (success) {
                std::cout << "  Done" << std::endl;
                logfile << "Success: " << filename << std::endl;
                success_count++;
            } else {
                logfile << "Failed: " << filename << std::endl;
                fail_count++;
            }
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        logfile << "Exception: " << e.what() << std::endl;
    }

    logfile << "Total: " << total << ", Success: " << success_count << ", Failed: " << fail_count << std::endl;
    logfile << "JPG: " << jpg_count << ", RAW: " << raw_count << std::endl;
    logfile << "Program finished\n";

    std::cout << "================================\n";
    std::cout << " Processing Complete\n";
    std::cout << "================================\n";
    std::cout << "Total: " << total << " files" << std::endl;
    std::cout << "Success: " << success_count << " files" << std::endl;
    std::cout << "Failed: " << fail_count << " files" << std::endl;
    std::cout << "JPG: " << jpg_count << ", RAW: " << raw_count << std::endl;

    return (fail_count == 0) ? 0 : 1;
}