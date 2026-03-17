/*
颜色阈值色块识别模块 
功能：
1 HSV颜色分割
2 形态学去噪
3 轮廓筛选
4 计算中心/面积
5 绘制检测结果
*/
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

//颜色参数
struct ColorRange
{
    Scalar lower1;
    Scalar upper1;

    Scalar lower2;
    Scalar upper2;

    Scalar drawColor;
    bool dualRange;   // 红色0,180
};

//轮廓信息参数
struct ContourInfo
{
    int id;

    Point center;

    double area;

    Rect bbox;

    Point circle_center;

    int circle_r;
};

//颜色阈值定义
ColorRange RED =
{
    Scalar(0,100,100),
    Scalar(10,255,255),

    Scalar(160,100,100),
    Scalar(179,255,255),

    Scalar(0,0,255),
    true
};

ColorRange BLUE =
{
    Scalar(100,100,100),
    Scalar(130,255,255),

    Scalar(),
    Scalar(),

    Scalar(255,0,0),
    false
};

ColorRange GREEN =
{
    Scalar(40,50,50),
    Scalar(80,255,255),

    Scalar(),
    Scalar(),

    Scalar(0,255,0),
    false
};

//通道转换
Mat convertToHSV(const Mat& image)
{
    Mat hsv;
    cvtColor(image, hsv, COLOR_BGR2HSV);
    return hsv;
}

//创建颜色掩码
Mat createColorMask(const Mat& hsv, const ColorRange& range)
{
    Mat mask;

    if(range.dualRange)
    {
        Mat mask1, mask2;

        inRange(hsv, range.lower1, range.upper1, mask1);
        inRange(hsv, range.lower2, range.upper2, mask2);
        //合并双阈值
        bitwise_or(mask1, mask2, mask);
    }
    else
    {
        inRange(hsv, range.lower1, range.upper1, mask);
    }

    return mask;
}

//形态学去噪
Mat applyMorphology(const Mat& mask, int kernelSize = 5)
{
    Mat result;

    Mat kernel = getStructuringElement(2,Size(kernelSize,kernelSize));//5*5 椭圆形

    morphologyEx(mask,result,MORPH_OPEN,kernel);//腐蚀膨胀 

    morphologyEx(result,result,MORPH_CLOSE,kernel);//膨胀腐蚀

    return result;
}

//轮廓筛选
void findValidContours(
        const Mat& mask,
        vector<vector<Point>>& validContours,
        vector<ContourInfo>& infos,
        double minArea = 100
)
{
    vector<vector<Point>> contours;

    findContours(mask,contours,RETR_EXTERNAL,CHAIN_APPROX_SIMPLE);

    int id = 1;

    for(auto& c: contours)
    {
        double area = contourArea(c);//计算轮廓面积

        if(area < minArea)
        {
            continue;
        }
        Moments m = moments(c);

        int cx = 0;
        int cy = 0;

        if(m.m00 != 0)
        {
            cx = m.m10/m.m00;
            cy = m.m01/m.m00;
        }

        //外接矩形
        Rect bbox = boundingRect(c);

        Point2f center;  
        float radius; 
        //外接圆
        minEnclosingCircle(c,center,radius); 

        ContourInfo info;

        info.id = id++;
        info.center = Point(cx,cy);
        info.area = area;
        info.circle_center = center;  
        info.circle_r = radius; 

        validContours.push_back(c);  
        infos.push_back(info);  
    }

    sort(infos.begin(),infos.end(),[](const ContourInfo&a, const ContourInfo&b){return a.area > b.area;});
}

//绘制检测结果
Mat drawResult(  
        const Mat& image,//输入图像  
        const vector<vector<Point>>& contours,  //所有检测到的轮廓
        const vector<ContourInfo>& infos,  //轮廓信息
        Scalar color  //所选颜色
) 
{
    Mat result = image.clone();//深拷贝
    // 画
    for(size_t i = 0;i<contours.size();i++)
    {
        //画轮廓
        drawContours(result,contours,i,color,2);//2为线宽

        Point p = infos[i].center;
        //画中心大红点
        circle(result,p,5,Scalar(0,0,255),-1);//5为半径,通道表示绘画颜色,-1表示填充
        //获取外接矩形
        Rect r = infos[i].bbox;

        rectangle(result,r,Scalar(255,0,0),2);//线宽2

        string text =  "ID:" 
        + to_string(infos[i].id) 
        +  " (" + to_string(p.x) 
        + "," + to_string(p.y) + ")"; 
                
        putText(result,  
                text,  
                Point(r.x,r.y-10),  
                FONT_HERSHEY_SIMPLEX,  
                0.5,  
                color,  
                2);           
        //字号0.5 线宽2       
    }

    return result;
}

//颜色检测流程 name:蓝色、绿色、红色
void detectColor(const Mat& image,  
                 const ColorRange& range,  
                 string name)  
{
    cout << "开始检测 " << name << endl;  

    //bgr -> hsv
    Mat hsv = convertToHSV(image);
    //创建颜色掩码
    Mat mask = createColorMask(hsv,range);
    //除噪
    Mat cleanMask = applyMorphology(mask);

    //找轮廓
    vector<vector<Point>> contours;//存轮廓边界点  
    vector<ContourInfo> infos;//存轮廓的其他信息

    findValidContours(cleanMask,contours,infos);
    //输出检测结果
    cout<<"发现"<<infos.size()<<"个"<<name<<"目标"<<endl;
    
    Mat result = drawResult(image,contours,infos,range.drawColor);  

    imshow(name + " mask", cleanMask);  
    imshow(name + " result", result);  
}

int main()  
{  
    Mat image = imread("../img/armor1.png");  
  
    if(image.empty())  
    {  
        cout<<"[错误]图像读取失败"<<endl;  
        return -1;  
    }  
  
    detectColor(image,RED,"red");  
  
    detectColor(image,BLUE,"blue");  
  
    detectColor(image,GREEN,"green");  
  
    waitKey(0);  
  
    return 0;  
}