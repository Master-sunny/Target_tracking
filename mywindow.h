#ifndef MYWINDOW_H
#define MYWINDOW_H

#include <QMainWindow>
#include <QString>
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"

using namespace cv;

namespace Ui {
class myWindow;
}

class myWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit myWindow(QWidget *parent = 0);
    ~myWindow();
    QImage toQimage(Mat src);

private slots:
    void on_Button2_clicked();

    void on_Button4_clicked();

    void on_Button3_clicked();

    void on_Button1_clicked();

private:
    Ui::myWindow *ui;
    QImage         qimg;
    QImage         qline;
    QString        filePath;
    Mat            frame;
    Mat            VGA_frame;
    Mat            background;
    bool           firstRead;
    bool           process_mode;
};

#endif // MYWINDOW_H
