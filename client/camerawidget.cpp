/**
 * @file camerawidget.cpp
 * @brief 摄像头实时画面组件实现
 */

#include "camerawidget.h"

#include <QCameraInfo>
#include <QResizeEvent>
#include <QPainter>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QMediaRecorder>
#include <QAudioEncoderSettings>
#include <QVideoEncoderSettings>

CameraWidget::CameraWidget(QWidget *parent)
    : QWidget(parent)
    , camera_(nullptr)
    , viewfinder_(nullptr)
    , image_capture_(nullptr)
    , media_recorder_(nullptr)
    , current_volume_(50)
{
    setStyleSheet("background: #000000; border-radius: 24px;");
    setAttribute(Qt::WA_StyledBackground, true);
    initCamera();
}

CameraWidget::~CameraWidget()
{
    if (media_recorder_ && media_recorder_->state() == QMediaRecorder::RecordingState) {
        media_recorder_->stop();
    }
    if (camera_ && camera_->state() == QCamera::ActiveState) {
        camera_->stop();
    }
}

void CameraWidget::initCamera()
{
    if (!findDefaultCamera()) {
        qWarning() << "No camera device found, showing placeholder";
        return;
    }
    viewfinder_ = new QCameraViewfinder(this);
    viewfinder_->setGeometry(0, 0, width(), height());
    viewfinder_->show();
    camera_->setViewfinder(viewfinder_);
    image_capture_ = new QCameraImageCapture(camera_, this);
    image_capture_->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    media_recorder_ = new QMediaRecorder(camera_, this);

    QAudioEncoderSettings audio_settings;
    audio_settings.setCodec("audio/pcm");
    audio_settings.setSampleRate(16000);
    audio_settings.setBitRate(64000);
    audio_settings.setChannelCount(1);
    media_recorder_->setAudioSettings(audio_settings);

    QVideoEncoderSettings video_settings;
    video_settings.setCodec("video/x-vp8");
    video_settings.setResolution(640, 480);
    video_settings.setFrameRate(30);
    video_settings.setBitRate(2500000);
    media_recorder_->setVideoSettings(video_settings);

    QString videos_dir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    QDir().mkpath(videos_dir);
    QString output_path = videos_dir + "/eye_friend_recording_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".webm";
    media_recorder_->setOutputLocation(QUrl::fromLocalFile(output_path));
    camera_->start();
}

bool CameraWidget::findDefaultCamera()
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
        return false;
    }
    const QCameraInfo *selected = &cameras.first();
    for (const QCameraInfo &info : cameras) {
        if (info.position() == QCamera::BackFace) {
            selected = &info;
            break;
        }
    }
    camera_ = new QCamera(*selected, this);
    return true;
}

void CameraWidget::capturePhoto()
{
    if (!image_capture_ || !camera_ || camera_->state() != QCamera::ActiveState) {
        qWarning() << "Cannot capture photo: camera not ready";
        return;
    }
    image_capture_->capture();
    QString pictures_dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir().mkpath(pictures_dir);
    QString photo_path = pictures_dir + "/eye_friend_photo_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".jpg";
    connect(image_capture_, &QCameraImageCapture::imageCaptured,
            this, [photo_path](int id, const QImage &preview) {
        Q_UNUSED(id);
        if (!preview.isNull()) {
            preview.save(photo_path);
            qDebug() << "Photo saved to:" << photo_path;
        }
    });
}

void CameraWidget::startRecording()
{
    if (!media_recorder_ || !camera_ || camera_->state() != QCamera::ActiveState) {
        qWarning() << "Cannot start recording: camera not ready";
        return;
    }
    if (media_recorder_->state() == QMediaRecorder::RecordingState) {
        qWarning() << "Already recording";
        return;
    }
    QString videos_dir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    QDir().mkpath(videos_dir);
    QString output_path = videos_dir + "/eye_friend_recording_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".webm";
    media_recorder_->setOutputLocation(QUrl::fromLocalFile(output_path));
    media_recorder_->record();
    qDebug() << "Recording started to:" << output_path;
}

void CameraWidget::stopRecording()
{
    if (media_recorder_ && media_recorder_->state() == QMediaRecorder::RecordingState) {
        media_recorder_->stop();
        qDebug() << "Recording stopped";
    }
}

void CameraWidget::setVolume(int volume)
{
    current_volume_ = qBound(0, volume, 100);
    qDebug() << "Volume set to:" << current_volume_;
}

void CameraWidget::switchCamera()
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.size() < 2) {
        return;
    }
    if (media_recorder_ && media_recorder_->state() == QMediaRecorder::RecordingState) {
        media_recorder_->stop();
    }
    camera_->stop();

    // 循环切换到下一个摄像头: 删除旧对象, 用新的 QCameraInfo 重建
    static int camera_index = 0;
    camera_index = (camera_index + 1) % cameras.size();

    delete image_capture_;
    delete media_recorder_;
    delete camera_;

    camera_ = new QCamera(cameras[camera_index], this);
    camera_->setViewfinder(viewfinder_);
    image_capture_ = new QCameraImageCapture(camera_, this);
    media_recorder_ = new QMediaRecorder(camera_, this);
    camera_->start();
}

void CameraWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (viewfinder_) {
        viewfinder_->setGeometry(0, 0, width(), height());
    }
}

void CameraWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (!camera_ || camera_->state() != QCamera::ActiveState) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0.0, QColor("#DCDCDC"));
        gradient.setColorAt(0.5, QColor("#E5E5E5"));
        gradient.setColorAt(1.0, QColor("#F0F0F0"));
        painter.fillRect(rect(), gradient);
        painter.setPen(QColor(120, 120, 120));
        QFont font("Inter", 14);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, "Camera Not Ready");
    }
}
