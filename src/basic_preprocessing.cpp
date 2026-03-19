/*
任务1:图像基础预处理模块

功能：
1 读取图片
2 灰度转换
3 高斯模糊去噪
4 直方图均衡化
5 输出对比图
*/

#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>

using namespace std;
using namespace cv;

//读取图像
Mat readImage(const string& image_path)
{
    Mat image = imread(image_path);

    if(image.empty())
    {
        cout << "[错误] 无法读取图像: " << image_path << endl;
        return Mat();
    }

    return image;
}

//灰度转换
Mat convertToGray(const Mat& image)
{
    Mat gray;

    cvtColor(image,gray,COLOR_BGR2GRAY);

    cout<<"灰度转换done"<< endl;

    return gray;
}

//高斯模糊
Mat gaussianBlur(const Mat& image)
{
    Mat blurred;

    GaussianBlur(image,blurred,Size(5,5),0);

    cout<<"高斯模糊done"<< endl;

    return blurred;
}

//直方图均衡化
Mat histogramEqualization(const Mat& image)
{
    Mat result;
    if(image.channels() == 1)
    {
        equalizeHist(image,result);
    }
    else
    {
        //彩色图像只对一个通道进行均衡化处理，不然会导致图像失真
        Mat yuv;//Y:亮度 U:色度 V:色度

        cvtColor(image,yuv,COLOR_BGR2YUV);
        //分离
        vector<Mat> channels;
        split(yuv,channels);

        equalizeHist(channels[0],channels[0]);//只改变亮度
        //合并
        merge(channels,yuv);

        cvtColor(yuv,result,COLOR_YUV2BGR);
    }

    cout<<"直方图均衡化done"<< endl;

    return result;
}

//创建对比图
Mat createComparison(
        const Mat& original,
        const vector<Mat>& images,//处理后的图像
        const vector<string>& titles//处理方法名
)
{
    vector<Mat> displayImages;//用来存最终要显示的图像

    displayImages.push_back(original);

    for(auto& img:images)
    {
        Mat temp;
        if(img.channels() == 1)
        {
            cvtColor(img,temp,COLOR_GRAY2BGR);
        }
        else
        {
            temp = img;
        }

        displayImages.push_back(temp);
    }
//统一尺寸
    vector<Mat> resized;

    int w = original.cols;
    int h = original.rows;

    for(auto& img : displayImages)
    {
        Mat temp;

        resize(img,temp,Size(w,h));

        resized.push_back(temp);
    }

    Mat comparison;
//拼接
    hconcat(resized,comparison);//水平拼接

    //备注
    for(size_t i = 0;i<titles.size();i++)
    {
        int x = i*w+10;
        int y = 30;
        //黑底白字
        putText(comparison,titles[i], Point(x,y),FONT_HERSHEY_SIMPLEX,0.6,Scalar(0,0,0),3);
        putText(comparison,titles[i], Point(x,y),FONT_HERSHEY_SIMPLEX,0.6,Scalar(255,255,255),2);
    }
    
    cout<<"对比图创建done"<<endl;

    return comparison;
}
//图像保存
bool saveImage(const Mat& image,const string& path)
{
    filesystem::create_directories(filesystem::path(path).parent_path());//确保保存图片的文件夹存在

    bool ok = imwrite(path,image);

    if(ok)
        cout<<"图像保存done "<<path<<endl;
    else
        cout<<"[错误] 图像保存失败 "<<path<<endl;

    return ok;
}

void runPipeline(const string& imagePath)
{
    cout << endl;
    cout << "====================================" << endl;
    cout << "图像基础预处理流程" << endl;
    cout << "====================================" << endl;

    Mat original = readImage(imagePath);

    if(original.empty())
        return;

    Mat gray = convertToGray(original);

    Mat blurred = gaussianBlur(gray);

    Mat equalized = histogramEqualization(gray);

    vector<Mat> list =
    {
        gray,
        blurred,
        equalized
    };

    vector<string> titles =
    {
        "Original",
        "Grayscale",
        "Gaussian Blur",
        "Equalized"
    };

    Mat comparison = createComparison(original,list,titles);

    saveImage(gray,"../img/output/grayscale.jpg");

    saveImage(blurred,"../img/output/blurred.jpg");

    saveImage(equalized,"../img/output/equalized.jpg");

    saveImage(comparison,"../img/output/comparison.jpg");

    cout << "====================================" << endl;
    cout << "预处理流程完成" << endl;
    cout << "====================================" << endl;

    imshow("comparison",comparison);
    
    waitKey(0); 
}

int main(int argc,char** argv)
{
    string path;

    if(argc>1)
        path = argv[1];
    else
        path = "/home/ubuntu/cpp/vision_test/img/armor1.png";

    runPipeline(path);

    return 0;
}