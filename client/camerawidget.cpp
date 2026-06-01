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
    , current_camera_index_(0)
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
    current_camera_index_ = 0;
    const QCameraInfo *selected = &cameras.first();
    for (int i = 0; i < cameras.size(); ++i) {
        if (cameras[i].position() == QCamera::BackFace) {
            selected = &cameras[i];
            current_camera_index_ = i;
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

void CameraWidget::requestFrame()
{
    if (!image_capture_ || !camera_ || camera_->state() != QCamera::ActiveState) {
        qWarning() << "Cannot capture frame: camera not ready";
        return;
    }
    // Qt 5.12 兼容的单发连接：堆上存连接句柄，槽执行后自毁
    QMetaObject::Connection * const connPtr = new QMetaObject::Connection;
    *connPtr = connect(image_capture_, &QCameraImageCapture::imageCaptured,
            this, [this, connPtr](int id, const QImage &preview) {
        disconnect(*connPtr);
        delete connPtr;
        Q_UNUSED(id)
        if (!preview.isNull()) {
            emit frameCaptured(preview);
        }
    });
    image_capture_->capture();
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
    int next = (current_camera_index_ + 1) % cameras.size();
    setCameraDevice(next);
}

void CameraWidget::setCameraDevice(int index)
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (index < 0 || index >= cameras.size()) {
        qWarning() << "Invalid camera index:" << index;
        return;
    }
    if (media_recorder_ && media_recorder_->state() == QMediaRecorder::RecordingState) {
        media_recorder_->stop();
    }
    if (camera_) {
        camera_->stop();
    }

    delete image_capture_;
    delete media_recorder_;
    delete camera_;

    current_camera_index_ = index;
    camera_ = new QCamera(cameras[index], this);
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

int CameraWidget::currentCameraIndex() const
{
    return current_camera_index_;
}

void CameraWidget::restartCamera()
{
    if (media_recorder_ && media_recorder_->state() == QMediaRecorder::RecordingState) {
        media_recorder_->stop();
    }

    if (camera_) {
        camera_->stop();
    }

    delete image_capture_;
    image_capture_ = nullptr;
    delete media_recorder_;
    media_recorder_ = nullptr;
    delete camera_;
    camera_ = nullptr;

    if (viewfinder_) {
        viewfinder_->deleteLater();
        viewfinder_ = nullptr;
    }

    initCamera();
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
