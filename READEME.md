# Focus Evaluate - 人脸虚焦检测

基于 SCRFD 2.5G ONNX 模型的专业相机照片人脸检测与虚焦评估系统。

## 功能

- **人脸检测**: SCRFD 2.5G 高精度检测，支持最高 6000x4000 分辨率
- **虚焦评估**: 基于拉普拉斯方差的双眼清晰度评分
- **眼睛开合度**: CLAHE + 自适应阈值 + 垂直投影对比度组合判定
- **一票否决**: 人脸占比 / 双眼检出 / 清晰度 / 开合度四重熔断

## 目录结构

```
├── main.cpp                      # 入口
├── compile_face.bat              # 编译脚本
├── config/FocusEvalConfig.h      # 所有阈值集中管理
├── model/
│   ├── FaceLandmark.h            # 检测结果（bbox + 5点关键点）
│   └── FocusResult.h             # 虚焦判定结果 + 状态码
├── utils/
│   ├── ImagePreprocess.h         # 下采样 + blob 预处理
│   ├── EyeRoiCropper.h           # 眼部 ROI 裁剪
│   └── SharpnessMetric.h/.cpp    # 拉普拉斯方差 + 眼睛开合度
├── detector/
│   ├── IFaceDetector.h           # 检测器抽象接口
│   └── ScrfdOnnxDetector.h/.cpp  # SCRFD 2.5G ONNX 实现
├── core/
│   └── EyeFocusEvaluator.h/.cpp  # 一票否决评估器（依赖注入）
├── render/ResultRenderer.h       # 标注框 / 关键点绘制
├── api/FocusApi.h/.cpp           # CLI 门面 + 批量处理
├── models/                       # ONNX 模型文件
└── after/                        # 输入图片目录
```

## 依赖

- **OpenCV 5.0** (core, imgcodecs, imgproc, dnn)
- **Visual Studio 2022** (MSVC v143, C++17)
- **SCRFD 2.5G** ONNX 模型文件放入 `models/`

## 编译

```batch
.\compile_face.bat
```

## 运行

```batch
# 批量处理 after/ 目录下所有图片（不保存标注图）
.\focus_evaluate.exe

# 保存标注图到 detection_result/
.\focus_evaluate.exe --save

# 指定输入目录
.\focus_evaluate.exe --input my_images --save
```

## 判定标准

| 等级 | 拉普拉斯方差 | 说明 |
|------|-------------|------|
| 严重虚焦 | < 100 | 不可用 |
| 轻度虚焦 | 100 - 500 | 勉强可用 |
| 基本清晰 | 500 - 1500 | 可用 |
| 清晰 | > 1500 | 优秀 |

一票否决条件：人脸占比 < 0.1% 或 > 85% / 双眼未检出 / 双眼模糊(lap < 80) / 眼睛闭合(open < 0.18)
