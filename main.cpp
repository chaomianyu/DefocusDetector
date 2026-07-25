/**
 * focus_evaluate 入口
 * 单文件 main，所有模块通过 FocusApi 门面组装
 */
#include "api/FocusApi.h"

int main(int argc, char* argv[]) {
    FocusApi app;
    if (!app.parse_args(argc, argv)) return 1;
    return app.run();
}
