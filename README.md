## 考核任务清单

### 一、基础必做任务

| 任务 | 描述 | 完成情况 |
|------|------|----------|
| 图像基础预处理 | 读取、灰度转换、高斯模糊、直方图均衡化 | 完成 |
| 颜色阈值色块识别 | HSV空间红/蓝色分割、形态学去噪、轮廓标注 | 完成 |
| 简单特征识别 | 矩形/圆形识别、0-9印刷体数字识别（无OCR库） | 形状识别不精准，数字识别无法正常进行 |

# OpenCV 图像处理项目 README

## 一、项目简介

本项目基于 OpenCV 实现了一个完整的图像处理流程，主要包括：

* 图像预处理（降噪、二值化）
* 颜色识别
* 数字识别（基于轮廓 + 模板匹配）

项目采用 C++ 实现，并通过 CMake 进行构建。

---

## 二、环境配置

### 1. 系统环境

* Ubuntu（推荐 20.04 / 22.04）

### 2. 安装基础工具

```bash
sudo apt update
sudo apt install build-essential cmake git
```

### 3. 安装 OpenCV

```bash
sudo apt install libopencv-dev
```

安装完成后可通过以下命令验证：

```bash
pkg-config --modversion opencv4
```

### 4. 项目结构示例

```
project/
├── CMakeLists.txt
├── src/
│   ├── basic_preprocessing.cpp
│   ├── color_detection.cpp
│   ├── shape_detection.cpp
├── img/
│   └──output/
│         ├── basic_preprocessing
│         ├── color_detection
│         └── shape_detection
│
└── build/
```

### 5. CMake 构建

```bash
mkdir build
cd build
cmake ..
make
```

生成可执行文件后运行：

```bash
./文件名
```

---

## 三、模块设计与实现思路

### 1. 图像预处理模块（preprocess）

#### 目标

提高图像质量，为后续识别提供稳定输入。

#### 主要步骤

1. 转灰度图
2. 高斯滤波（去噪）
3. 二值化（自适应阈值或固定阈值）

#### 核心函数

* `cvtColor()`：彩色转灰度
* `GaussianBlur()`：降噪
* `threshold()` / `adaptiveThreshold()`：二值化

#### 思路总结

通过降噪减少干扰，利用二值化突出目标轮廓，使后续轮廓检测更加稳定。

---

### 2. 颜色识别模块（color_detect）

#### 目标

识别图像中特定颜色区域（如红色、蓝色）。

#### 主要步骤

1. BGR 转 HSV 空间
2. 设定颜色范围（HSV阈值）
3. 掩膜提取目标区域
4. 形态学操作优化结果

#### 核心函数

* `cvtColor()`：BGR → HSV
* `inRange()`：颜色筛选
* `erode()` / `dilate()`：形态学处理

#### 思路总结

HSV 空间对颜色更敏感，通过阈值分割可以稳定提取目标颜色区域。

---

### 3. 数字识别模块（digit_recognize）

#### 目标

识别图像中的 0-9 数字（不使用 OCR）。

#### 核心思路

基于：

* 轮廓检测
* ROI 提取
* 模板匹配

#### 实现流程

1. 在二值图上进行轮廓检测：

   ```cpp
   findContours()
   ```

2. 筛选有效轮廓（根据面积/长宽比）

3. 提取 ROI（感兴趣区域）

4. 统一尺寸（resize）

5. 与模板进行匹配：

   ```cpp
   matchTemplate()
   ```

6. 通过最小误差确定数字类别：

   ```cpp
   minMaxLoc()
   ```

#### 模板生成

* 使用 `putText()` 生成 0-9 标准模板
* 存储在 `map<int, Mat>` 中

#### 思路总结

通过模板匹配计算相似度，实现轻量级数字识别，适用于规则字体场景。

---

## 四、整体流程

```
输入图像
   ↓
图像预处理
   ↓
颜色筛选
   ↓
轮廓检测
   ↓
ROI 提取
   ↓
数字识别
   ↓
输出结果（标注图像）
```

---

## 五、常见问题

### 1. 找不到可执行文件

* 确认是否执行了 `make`
* 检查 `CMakeLists.txt` 是否正确

### 2. 修改代码后如何重新编译

```bash
cd build
make
```

### 3. 识别效果不好

* 调整阈值
* 优化模板尺寸
* 增加轮廓筛选条件
