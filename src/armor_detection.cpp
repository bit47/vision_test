#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

#include <cmath>
#include <filesystem>

using namespace cv;
using namespace std;

struct LightBar {
    Point center;
    float width;
    float height;
    float angle;
    float area;
    vector<Point> box;
    vector<Point> contour;
    float brightness;
};
 
struct Armor {
    LightBar left_bar;
    LightBar right_bar;
    Point center;
    float angle;
    float width;
    float height;
    float confidence;
    vector<Point> box;
    vector<Point> corners;
};

class ArmorDetector
{
public:
//initialize
    ArmorDetector()
    :blue_lower(Scalar(100,120,100))
    ,blue_upper(Scalar(130,255,255))
    ,red_lower1(Scalar(0,120,100))
    ,red_upper1(Scalar(10,255,255))
    ,red_lower2(Scalar(160,120,100))
    ,red_upper2(Scalar(179,255,255))
    ,min_aspect_ratio(1.5)
    ,max_aspect_ratio(4.0)
    ,min_area(200)
    ,max_area(5000)
    ,max_angle_diff(15)
    ,max_height_diff_ratio(0.3)
    ,max_center_dist_ratio(4.0)
    {}

    void preprocess(const Mat& image,Mat& gray,Mat& blurred,Mat& hsv)
    {
        cvtColor(image,gray,COLOR_BGR2GRAY);
        GaussianBlur(gray,blurred,Size(5,5),0);
        cvtColor(image,hsv,COLOR_BGR2HSV);
    }

    Mat detectColor(const Mat& hsv,string color)
    {
        Mat mask;

        if(color == "red")
        {
            Mat m1,m2;
            inRange(hsv,red_lower1,red_upper1,m1);
            inRange(hsv,red_lower2,red_upper2,m2);
            bitwise_or(m1,m2,mask);
        }
        else
        {
            inRange(hsv,blue_lower,blue_upper,mask);
        }

        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5,5));
        morphologyEx(mask,mask,MORPH_CLOSE,kernel);
        morphologyEx(mask,mask,MORPH_OPEN,kernel);

        return mask;
    }

    vector<LightBar> findLightBars(const Mat& mask, const Mat& gray)
    {
        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        vector<LightBar> bars;
        for(auto& contour:contours)
        {
            float area = contourArea(contour);
            if(area<min_area||area>max_area) continue;
            
            //最小外接旋转矩形
            RotatedRect rect = minAreaRect(contour);

            Point2f pts[4];
            rect.points(pts);
            float width = rect.size.width;
            float height = rect.size.height;
            float angle = rect.angle;

            if (width < height) 
            {
                swap(width, height);
                angle += (angle < 0 ? 90 : -90);
            }

            float ratio = width / height;
            if(ratio < min_aspect_ratio || ratio > max_aspect_ratio) continue;

            Mat roi_mask = Mat::zeros(mask.size(),CV_8UC1);
            drawContours(roi_mask,vector<vector<Point>>{contour},-1,Scalar(255),-1);
            
            //计算亮度
            Scalar mean_val = mean(gray,roi_mask);//只计算roi_mask 中“白色区域”的像素;Scalar 本质是一个 4 维数组
            // mean_val[0] → B 通道平均值
            // mean_val[1] → G 通道平均值
            // mean_val[2] → R 通道平均值
            //过滤颜色对但不发光的区域
            if(mean_val[0]<50) continue;

            //push
            LightBar bar;
            bar.center = rect.center;
            bar.width = width;
            bar.height = height;
            bar.angle = angle;
            bar.area = area;
            bar.contour = contour;
            bar.brightness = mean_val[0];

            for (int i = 0; i < 4; i++)
                bar.box.push_back(pts[i]);
    
            bars.push_back(bar);
        }  
        sort(bars.begin(), bars.end(),
        [](const LightBar& a, const LightBar& b)
        {
            return a.center.x < b.center.x;
        });
        
        return bars;
    }

// 置信度计算
// 总分 = 角度评分 + 高度评分 + 距离评分
// 权重：
// 角度 40% + 高度 30% + 距离 30%
    float calConfidence(const LightBar& b1, const LightBar& b2, 
                        float angle_diff, float height_diff, float dist)//角度差、高度差、中心距离
    {
//1.角度评分-是否平行
        float angle_score = 1 - angle_diff/max_angle_diff;
//2.高度评分-是否等高
        float avg_height = (b1.height+b2.height)/2;
        float height_score = 1 - height_diff/(avg_height*max_height_diff_ratio);
//3.距离评分-灯条间距 / 灯条高度 ≈ 固定比例（装甲板结构决定）
        float optimal = 2.5;
        float ratio = dist/avg_height;
        float dist_score = max(0.f,(1-abs(ratio/optimal)));

        return angle_score*0.4 + height_score*0.3 + dist_score*0.3;
    }


// IoU筛选去重 1->重合 0->不重合
    float calIoU(const Armor& a1, const Armor& a2)
    {
        //计算欧氏距离
        float dx = a1.center.x - a2.center.x;
        float dy = a1.center.y - a2.center.y;

        float dist = sqrt(dx*dx + dy*dy);

        //设定重合判定的阈值
       float avg = (a1.width + a2.width)/4;

       return (dist<avg)?(1-dist/avg):0;
    }

//NMS依据置信度顺序重合判断
    vector<Armor> NMS(vector<Armor> armors)
    {
        vector<Armor> res;

        for(auto a:armors)
        {
            bool flag = true;
            for(auto r:res)
            {
                if(calIoU(a,r)>0.5)
                {
                    flag = false;
                    break;
                }
            }
            if(flag) res.push_back(a);
        }

        return res;
    }
//装甲板匹配 角度差,高度差,灯条距离判断 为了调用 calConfidence(
//                                                      const LightBar& b1, const LightBar& b2, 
//                                                      float angle_diff, float height_diff, float dist)
    vector<Armor> matchArmor(vector<LightBar>& bars)
    {
        vector<Armor> armors;

        for(int i = 0;i<bars.size();i++)
        {
            for(int j = i+1;j<bars.size();j++)
            {
                LightBar b1 = bars[i];
                LightBar b2 = bars[j];
                
                float angle_diff = abs(b1.angle-b2.angle); 
                if(angle_diff>max_angle_diff) continue;

                float height_diff = abs(b1.height-b2.height);
                float avg_h = (b1.height + b2.height)/2;
                if(height_diff/avg_h > max_height_diff_ratio) continue;

                float dx = b1.center.x - b2.center.x;
                float dy = b1.center.y - b2.center.y;
                float dist = sqrt(dx*dx + dy*dy);
                if(dist/avg_h > max_center_dist_ratio) continue;//归一化

                Armor armor;
                armor.left_bar = b1;
                armor.right_bar = b2;

                armor.center = Point((b1.center.x + b2.center.x)/2,(b1.center.y + b2.center.y)/2);

                armor.angle = atan2(dy,dx) * 180/CV_PI;//弧度->角度
                armor.width = dist;
                armor.height = avg_h;

                armor.confidence = calConfidence(b1,b2,angle_diff,height_diff,dist);

                armors.push_back(armor);
            }
        }

        sort(armors.begin(),armors.end(),[](Armor& a,Armor& b){return a.confidence > b.confidence;});

        return NMS(armors);
    }
    //主流程    
    vector<Armor> detect(const Mat& image, string color = "blue")
    {
        //预处理
        Mat gray,blurred,hsv;
        preprocess(image,gray,blurred,hsv);
        //create mask
        Mat mask = detectColor(hsv,color);
        //find light bar
        auto bars = findLightBars(mask,gray);
        //match armor
        auto armors = matchArmor(bars);

        return armors;
    }

private:
//color split
    Scalar blue_lower,blue_upper;
    Scalar red_lower1,red_upper1,red_lower2,red_upper2;
//lightbar selection
    float min_aspect_ratio,max_aspect_ratio;
//area limitation 
    float min_area,max_area;
//armor match parameter
    float max_angle_diff;//最大角度差:两个灯条必须接近平行
    float max_height_diff_ratio;//高度差比例:左右灯条厚度要接近,因为倾斜对厚度的影响不大,方便比较
    float max_center_dist_ratio;//中心距离比例:两个灯条不能离太远
};

int main(int argc,char** argv)
{
    string image_path;
    if(argc > 1) image_path = argv[1];   
    else  image_path = "../img/armor_test.jpg";   

    string color;
    if(argc > 2 && (argv[2] == "blue" || argv[2] == "red"))color = argv[2];
    else
    {
        cerr<<"[错误]dont have the option"<<argv[2]<<endl;
        return 0;
    }

    Mat img = imread(image_path);
    if(img.empty())
    {
        cerr<<"[错误]图像读取失败"<<endl;
        return -1;
    }

    ArmorDetector armorDetector;

    armorDetector.detect(img,color);

    return 0;
}