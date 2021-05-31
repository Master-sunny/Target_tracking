#include "mywindow.h"
#include "ui_mywindow.h"
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <QFileDialog>
#include <QImage>
#include "opencv2/tracking.hpp"
#include "QString"

int hmin = 0;
int smin = 0;
int vmin = 0;
int hmax = 180;
int vmax = 80;
int smax = 80;
int bins = 16;
Point2f shiftdox;

using namespace cv;
using namespace std;
Mat video_first_image;
Mat show_first_image;
Mat cam_first_image;

Mat temp;
Mat    dst;
myWindow::myWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::myWindow)
{
    ui->setupUi(this);
}

myWindow::~myWindow()
{
    delete ui;
}
//打开视频文件并显示视频第一帧
void myWindow::on_Button2_clicked()
{
    process_mode=0;
    VideoCapture capture;   
    filePath=QFileDialog::getOpenFileName(
                this,
                "openImage",
                "../"
                );
    capture.open(filePath.toLatin1().data());
    if(capture.isOpened()){
        capture.read(video_first_image);
        cout<<"ok1"<<endl;
    }
    qimg = toQimage(video_first_image);

    background=Mat::zeros(video_first_image.rows,video_first_image.cols,CV_8UC3);
    qline = toQimage(background);
    ui->label3->setPixmap(QPixmap::fromImage(qimg));
    ui->label2->setPixmap(QPixmap::fromImage(qline));
    cout<<"ok2"<<endl;
    capture.release();
}

//执行视频处理操作
void myWindow::on_Button3_clicked()
{
    VideoCapture capture;
    bool firstRead = true;
    float hrange[] = { 0, 180 };
    const float* hranges = hrange;
    Rect selection;
    Mat  hsv, hue, mask, hist, backprojection;
    Mat drawImg = Mat::zeros(300, 300, CV_8UC3);
    //以视频文件的方式处理并对应滤波参数
    if(process_mode==0){
        capture.open(filePath.toLatin1().data());
        hmin = 0;
        smin = 0;
        vmin = 0;
        hmax = 180;
        vmax = 100;
        smax = 90;
    }
    //以摄像头方式进行处理
    if(process_mode==1){
        capture.open(0);
        hmin = 35;
        smin = 43;
        vmin = 46;
        hmax = 77;
        vmax = 255;
        smax = 255;
    }
    while (capture.read(frame)) {
        //对于第一帧选取ROI
        if (firstRead) {
            Rect2d first = selectROI(frame);
            selection.x = first.x;
            selection.y = first.y;
            shiftdox.x=first.x;
            shiftdox.y=first.y;
            selection.width = first.width;
            selection.height = first.height;
            //printf("ROI.x= %d, ROI.y= %d, width = %d, height= %d", selection.x, selection.y, selection.width, selection.height);
        }
        //转换成HSV格式用来进行滤波和统计直方图
        cvtColor(frame, hsv, COLOR_BGR2HSV);
        //滤波
        inRange(hsv, Scalar(hmin, smin, vmin), Scalar(hmax, smax, vmax), mask);
        hue = Mat(hsv.size(), hsv.depth());
        int channels[] = { 0, 0 };
        mixChannels(&hsv, 1, &hue, 1, channels, 1);
        //绘制直方图
        if (firstRead) {
            // ROI 直方图计算
            Mat roi(hue, selection);
            Mat maskroi(mask, selection);
            //函数输出的hist是直方图的结果，一个二维数组，每个bin对应的数量，再做0到255的归一化
            calcHist(&roi, 1, 0, maskroi, hist, 1, &bins, &hranges);
            normalize(hist, hist, 0, 255, NORM_MINMAX);
            // 显示直方图
            int binw = drawImg.cols / bins;
            Mat colorIndex = Mat(1, bins, CV_8UC3);
            for (int i = 0; i < bins; i++) {
                 colorIndex.at<Vec3b>(0, i) = Vec3b(saturate_cast<uchar>(i * 180 / bins), 255, 255);
            }
            cvtColor(colorIndex, colorIndex, COLOR_HSV2BGR);
            for (int i = 0; i < bins; i++) {
                 //hist.at(i)是指第i个bin
                 int  val = saturate_cast<int>(hist.at<float>(i)*drawImg.rows / 255);//对255进行归一化 rows是图像的行
                 rectangle(drawImg, Point(i*binw, drawImg.rows), Point((i + 1)*binw, drawImg.rows - val), Scalar(colorIndex.at<Vec3b>(0, i)), -1, 8, 0);
            }
        }
        // 直方图反射投影
        calcBackProject(&hue, 1, 0, hist, backprojection, &hranges);
        // CAMShift tracking 使用滤波器滤波
        backprojection &= mask;
        RotatedRect trackBox = CamShift(backprojection, selection, TermCriteria((TermCriteria::COUNT | TermCriteria::EPS), 10, 1));
        // draw location on frame;
        ellipse(frame, trackBox, Scalar(0, 0, 255), 3, 8);
        line(background,shiftdox,trackBox.center,Scalar(0,255,0),3,8);
        shiftdox=trackBox.center;
        if (firstRead) {
            firstRead = false;
        }
        //在UI上显示实时结果
        qimg = toQimage(frame);
        qline = toQimage(background);
        ui->label3->setPixmap(QPixmap::fromImage(qimg));
        ui->label2->setPixmap(QPixmap::fromImage(qline));
        waitKey(50);
    }
        capture.release();
}
//Mat转换成QImage
void myWindow::on_Button4_clicked()
{
    this->close();
}

QImage myWindow::toQimage(Mat src)
{
    QImage img;
    //cv::resize(src,src,Size(320,240));
    if(src.channels()==3){
        cvtColor(src,src,CV_RGB2BGR);
        img=QImage((const unsigned char*)(src.data), src.cols, src.rows, QImage::Format_RGB888 );
    }
    else if(src.channels()==1){
        img=QImage((const unsigned char*)(src.data), src.cols, src.rows, QImage::Format_Grayscale8 );
    }
    else{
        cout<<"can not covert this picture to QImage"<<endl;
    }
    return img;
}
//通过摄像头
void myWindow::on_Button1_clicked()
{
    process_mode=1;
    VideoCapture capture;
    capture.open(0);
    if(capture.isOpened()){
        waitKey(100);
        capture.read(cam_first_image);
    }
    qimg = toQimage(cam_first_image);

    background=Mat::zeros(cam_first_image.cols,cam_first_image.rows,CV_8UC3);
    qline = toQimage(background);
    ui->label3->setPixmap(QPixmap::fromImage(qimg));
    ui->label2->setPixmap(QPixmap::fromImage(qline));
    capture.release();
}
