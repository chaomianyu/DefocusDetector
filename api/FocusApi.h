#pragma once
#include <string>
#include <vector>
#include "../config/FocusEvalConfig.h"
#include "../model/FocusResult.h"

/**
 * 对外门面 API
 * 提供批量处理、命令行解析、日志输出等完整入口
 */
class FocusApi {
public:
    FocusApi();

    // 命令行参数解析
    bool parse_args(int argc, char* argv[]);

    // 运行批量处理
    int run();

private:
    FocusEvalConfig cfg_;
    bool show_help_ = false;

    void print_usage() const;
    void print_header() const;
    void process_directory();
};
