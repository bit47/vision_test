/*
特征识别模块
功能：
1 识别图片中的几何图形
2 识别0-9印刷体数字
*/

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <filesystem>

using namespace std;
using namespace cv;

// 图形信息
struct ShapeInfo
{
    int id;
    string type;
    Point center;
    double area;
    Rect bbox;
    vector<Point> contour;
};

//图像预处理 二值图:轮廓检测  边缘图:几何分析
void preprocessForRecognition(
    const Mat& image, 
    Mat& binary, 
    Mat& edges)
{
    Mat gray;
//转灰度图
    if (image.channels() == 3)
    {
        cvtColor(image, gray, COLOR_BGR2GRAY);
    }
        
    else
    {
         gray = image.clone();
    }
       
    Mat blurred;
    GaussianBlur(gray, blurred, Size(5,5), 0);

    // 自适应阈值-二值化 
    //参数:输入 输出 最大像素值 
    //阈值计算方式(局部高斯平均) 
    //反向二值化(使目标为白色背景为黑色) 
    //局部窗口大小(11*11) 
    //阈值调整常数(用于调节亮度threshold = mean - 2)
    adaptiveThreshold(
        blurred,
        binary,
        255,
        ADAPTIVE_THRESH_GAUSSIAN_C,
        THRESH_BINARY_INV,
        11,
        2
    );

    Canny(blurred, edges, 50, 150);
//梯度 > 150 → 强边缘
// 50 < 梯度 <150 → 弱边缘
// <50 → 非边缘
 }

 //根据图形类型返回对应的绘制颜色。
 Scalar getShapeColor(const string& type)
{
    map<string, Scalar> colors =
    {
        {"Triangle", Scalar(0,255,255)},
        {"Rectangle", Scalar(0,255,0)},
        {"Square", Scalar(255,255,0)},
        {"Pentagon", Scalar(255,0,255)},
        {"Hexagon", Scalar(255,165,0)},
        {"Circle", Scalar(0,0,255)},
        {"Ellipse", Scalar(255,0,0)},
        {"Polygon", Scalar(128,128,128)},
        {"Unknown", Scalar(255,255,255)}
    };

    if(colors.count(type))//检查该类型是否存在
        return colors[type];

    return Scalar(255,255,255);
}

//形状归类
string classifyShape(
    const vector<Point>& approx,
    double area,
    double perimeter,
    double aspect_ratio
)//近似轮廓, 面积, 周长, 宽高比
{
    int vertices = approx.size();//顶点个数,圆形的近似轮廓顶点数量通常 > 6

    //计算圆度,判断是不是圆形
    double circularity = 0;
    //公式:circularity = 4*pi*A / P²
    if(perimeter > 0)
    {
        circularity = 4 * CV_PI * area / (perimeter * perimeter);
    }
    

    if(vertices == 3)
        return "Triangle";

    else if(vertices == 4)
    {
        if(aspect_ratio > 0.9 && aspect_ratio < 1.1)//对比宽高比
            return "Square";
        else
            return "Rectangle";
    }

    else if(vertices == 5)
        return "Pentagon";//五边形

    else if(vertices == 6)
        return "Hexagon";//六边形

    else if(vertices > 6)
    {
        if(circularity > 0.75)
            return "Circle";
        else if(aspect_ratio < 0.8 || aspect_ratio > 1.2)//对比宽高比
            return "Ellipse";//椭圆
        else
            return "Polygon";//多边形
    }

    return "Unknown";
}

//几何图形检测
vector<ShapeInfo> detectShapes(
        const Mat& image,
        const Mat& binary,
        Mat& annotated)//标注图
{
    cout << "\n开始[几何图形检测]\n";
//形态学操作
    Mat morph;

    Mat kernel = Mat::ones(3,3,CV_8U);

    morphologyEx(binary, morph, MORPH_CLOSE, kernel);
//轮廓检测
    vector<vector<Point>> contours;

    findContours(
        morph,
        contours,
        RETR_EXTERNAL,
        CHAIN_APPROX_SIMPLE
    );

    annotated = image.clone();//创建标注图

    vector<ShapeInfo> shapes;

    for(size_t i=0;i<contours.size();i++)
    {
        double area = contourArea(contours[i]);
        //过滤噪声
        if(area < 100)
            continue;
        
        double perimeter = arcLength(contours[i], true);//参数:轮廓,是否为闭合曲线

        vector<Point> approx;
        //轮廓近似 参数epsilon = 0.04 * perimeter
        approxPolyDP(
            contours[i],
            approx,
            0.04 * perimeter,
            true
        );
        
        //计算包围矩形
        Rect rect = boundingRect(approx);

        double aspect_ratio = (double)rect.width / rect.height;
        //判断形状
        string type = classifyShape(
                approx,
                area,
                perimeter,
                aspect_ratio
        );

        Moments m = moments(contours[i]);

        int cx, cy;
        //计算中心点坐标
        if(m.m00 != 0)
        {
            cx = int(m.m10/m.m00);
            cy = int(m.m01/m.m00);
        }
        else
        {
            cx = rect.x + rect.width/2;
            cy = rect.y + rect.height/2;
        }
//存储形状信息
        ShapeInfo info;

        info.id = shapes.size()+1;
        info.type = type;
        info.center = Point(cx,cy);
        info.area = area;
        info.bbox = rect;
        info.contour = contours[i];

        shapes.push_back(info);

        Scalar color = getShapeColor(type);

        drawContours(
            annotated,
            vector<vector<Point>>{approx},
            -1,
            color,
            2
        );

        circle(annotated, Point(cx,cy),5,Scalar(0,0,255),-1);

        putText(
            annotated,
            type,
            Point(rect.x,rect.y-10),
            FONT_HERSHEY_SIMPLEX,
            0.6,
            color,
            2
        );

        cout<<" 图形 "<<info.id<<" : "<<type<<endl;
    }

    cout<<"检测完成 "<<shapes.size()<<" 个图形\n";

    return shapes;
    
}

//创建数字模板
map<int, Mat> createDigitTemplates()//这名字怎么也这么长...
{
    //创建模板对象
    map<int,Mat> templates;

    //设置模板大小
    int w = 50;
    int h = 70;
    
    //生成0-9的数字模板
    for(int number = 0;number<10;number++)
    {
        //创建空白图像
        Mat temp = Mat::zeros(h,w,CV_8UC1);
        //创建图像内数字
        string text = to_string(number);

        int baseline=0;//用于文字排版的基线
        //确定文字位置 参数：内容、字体类型、缩放比例、线宽、*基线（返回字体基线高度
        //返回文字高度和文字宽度
        Size textSize = getTextSize(
                text,
                FONT_HERSHEY_SIMPLEX,
                2,
                3,
                &baseline
        );
        //让文字居中
        int x = (w - textSize.width)/2;
        int y = (h + textSize.height)/2;

        putText(
            temp,
            text,
            Point(x,y),
            FONT_HERSHEY_SIMPLEX,
            2,
            255,
            3
        );
        templates[number] = temp;
    }

    return templates;
}


//数字识别结构体
struct DigitInfo
{
    int digit;
    double confidence;
    Rect bbox;
};

//数字识别,找到所有数字轮廓
vector<DigitInfo> recognizeDigits(
        const Mat& image,
        const Mat& binary,
        map<int,Mat>& templates,
        Mat& annotated)
{
    cout<<"\n开始[数字识别]\n";

    vector<vector<Point>> contours;

    findContours(
        binary,
        contours,
        RETR_EXTERNAL,
        CHAIN_APPROX_SIMPLE
    );

    annotated = image.clone();

    vector<DigitInfo> digits;

    for(auto& c:contours)
    {
        Rect r = boundingRect(c);

        double area = contourArea(c);

        //筛选有效轮廓,太小的轮廓直接忽略
        if(r.width < 10 || r.height < 20 || area < 100)
            continue;
        
//提取ROI （region of interest）
        //截取数字区域
        Mat roi = binary(r);

        //统一尺寸，不然匹配函数无法比较
        resize(roi, roi, Size(50,70));//模板大小 50*70

//初始化最佳匹配

        int best_digit = -1;
        double best_score = 1e9;

        for(auto& item:templates)
        {
            Mat result;

            matchTemplate(
                roi,
                item.second,
                result,
                TM_SQDIFF_NORMED
            );//从roi中与模板匹配，输出到result中，用TM_SQDIFF_NORMED方法

            //找到最小匹配误差
            double minVal;

            minMaxLoc(result, &minVal);//参数二用于存储最小值地址

            //更新最佳匹配
            if(minVal < best_score)
            {
                best_score = minVal;
                best_digit = item.first;
            }
        }
//信息写入
        if(best_score < 0.3)
        {
            DigitInfo info;

            info.digit = best_digit;
            info.confidence = 1 - best_score;
            info.bbox = r;

            digits.push_back(info);

            rectangle(
                annotated,
                r,
                Scalar(0,255,0),
                2
            );

            putText(
                annotated,
                to_string(best_digit),
                Point(r.x,r.y-10),
                FONT_HERSHEY_SIMPLEX,
                0.8,
                Scalar(0,0,255),
                2
            );

            cout<<"识别数字 "<<best_digit
                <<" 置信度 "<<1-best_score<<endl;
        }
    }

    cout<<"数字识别完成 "<<digits.size()<<" 个\n";

    return digits;
}

void runShapeRecognition(string image_path)
{
    cout<<"\n开始特征识别\n";

    Mat image = imread(image_path);

    if(image.empty())
    {
        cout<<"[错误]读取图像失败\n";
        return;
    }

    Mat binary, edges;

    preprocessForRecognition(image, binary, edges);

    Mat shape_img;

    vector<ShapeInfo> shapes =
        detectShapes(image, binary, shape_img);

    map<int,Mat> templates =
        createDigitTemplates();

    Mat digit_img;

    vector<DigitInfo> digits =
        recognizeDigits(image, binary, templates, digit_img);

    imwrite("../img/output/shapes.jpg", shape_img);
    imwrite("../img/output/digits.jpg", digit_img);

    cout<<"===== 识别完成 =====\n";
}

int main(int argc,char** argv)
{
    string path="../img/output/preprocessor.png";

    if(argc>1)
        path=argv[1];

    runShapeRecognition(path);

    return 0;
}