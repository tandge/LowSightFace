/**
 * @file camerawidget.h
 * @brief 摄像头实时画面组件
 *
 * 使用 Qt Multimedia 模块实现摄像头画面采集与显示。
 * 支持拍照截图、视频录制功能。
 */

#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QWidget>
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QMediaRecorder>

class CameraWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CameraWidget(QWidget *parent = nullptr);
    ~CameraWidget();

    void capturePhoto();
    void startRecording();
    void stopRecording();
    void setVolume(int volume);
    void switchCamera();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void initCamera();
    bool findDefaultCamera();

    QCamera *camera_;
    QCameraViewfinder *viewfinder_;
    QCameraImageCapture *image_capture_;
    QMediaRecorder *media_recorder_;
    int current_volume_;
};

#endif // CAMERAWIDGET_H
