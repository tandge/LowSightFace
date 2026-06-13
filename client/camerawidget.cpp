/**
 * @file camerawidget.cpp
 * @brief 摄像头实时画面组件实现 (Qt6)
 */

#include "camerawidget.h"

#include <QMediaDevices>
#include <QResizeEvent>
#include <QPainter>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QVideoFrame>

CameraWidget::CameraWidget(QWidget *parent)
    : QWidget(parent)
    , camera_(nullptr)
    , capture_session_(nullptr)
    , image_capture_(nullptr)
    , media_recorder_(nullptr)
    , video_sink_(nullptr)
    , current_volume_(50)
    , current_camera_index_(0)
{
    setStyleSheet("background: #000000; border-radius: 24px;");
    setAttribute(Qt::WA_StyledBackground, true);
    initCamera();
}

CameraWidget::~CameraWidget()
{
    if (media_recorder_ && media_recorder_->recorderState() == QMediaRecorder::RecordingState) {
        media_recorder_->stop();
    }
    if (camera_ && camera_->isActive()) {
        camera_->stop();
    }
}

void CameraWidget::initCamera()
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        qWarning() << "No camera device found, showing placeholder";
        update();
        return;
    }

    if (current_camera_index_ < 0 || current_camera_index_ >= cameras.size()) {
        current_camera_index_ = 0;
        for (int i = 0; i < cameras.size(); ++i) {
            if (cameras[i].position() == QCameraDevice::BackFace) {
                current_camera_index_ = i;
                break;
            }
        }
    }

    capture_session_ = new QMediaCaptureSession(this);
    camera_ = new QCamera(cameras[current_camera_index_], this);
    video_sink_ = new QVideoSink(this);
    image_capture_ = new QImageCapture(this);
    media_recorder_ = new QMediaRecorder(this);

    connect(video_sink_, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        if (!frame.isValid()) {
            return;
        }
        QImage image = frame.toImage();
        if (!image.isNull()) {
            current_frame_ = image;
            update();
        }
    });

    capture_session_->setCamera(camera_);
    capture_session_->setVideoOutput(video_sink_);
    capture_session_->setImageCapture(image_capture_);
    capture_session_->setRecorder(media_recorder_);

    camera_->start();
}

void CameraWidget::capturePhoto()
{
    if (!image_capture_ || !camera_ || !camera_->isActive()) {
        qWarning() << "Cannot capture photo: camera not ready";
        return;
    }

    const QString pictures_dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir().mkpath(pictures_dir);
    const QString photo_path = pictures_dir + "/eye_friend_photo_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".jpg";
    image_capture_->captureToFile(photo_path);
    qDebug() << "Photo capture requested:" << photo_path;
}

void CameraWidget::requestFrame()
{
    if (!image_capture_ || !camera_ || !camera_->isActive()) {
        qWarning() << "Cannot capture frame: camera not ready";
        return;
    }

    QMetaObject::Connection * const connPtr = new QMetaObject::Connection;
    *connPtr = connect(image_capture_, &QImageCapture::imageCaptured,
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
    if (!media_recorder_ || !camera_ || !camera_->isActive()) {
        qWarning() << "Cannot start recording: camera not ready";
        return;
    }
    if (media_recorder_->recorderState() == QMediaRecorder::RecordingState) {
        qWarning() << "Already recording";
        return;
    }

    const QString videos_dir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    QDir().mkpath(videos_dir);
    const QString output_path = videos_dir + "/eye_friend_recording_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".mp4";
    media_recorder_->setOutputLocation(QUrl::fromLocalFile(output_path));
    media_recorder_->record();
    qDebug() << "Recording started to:" << output_path;
}

void CameraWidget::stopRecording()
{
    if (media_recorder_ && media_recorder_->recorderState() == QMediaRecorder::RecordingState) {
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
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (cameras.size() < 2) {
        return;
    }
    const int next = (current_camera_index_ + 1) % cameras.size();
    setCameraDevice(next);
}

void CameraWidget::setCameraDevice(int index)
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (index < 0 || index >= cameras.size()) {
        qWarning() << "Invalid camera index:" << index;
        return;
    }

    if (media_recorder_ && media_recorder_->recorderState() == QMediaRecorder::RecordingState) {
        media_recorder_->stop();
    }
    if (camera_) {
        camera_->stop();
    }

    delete capture_session_;
    capture_session_ = nullptr;
    delete image_capture_;
    image_capture_ = nullptr;
    delete media_recorder_;
    media_recorder_ = nullptr;
    delete video_sink_;
    video_sink_ = nullptr;
    delete camera_;
    camera_ = nullptr;

    current_camera_index_ = index;
    current_frame_ = QImage();
    capture_session_ = new QMediaCaptureSession(this);
    camera_ = new QCamera(cameras[index], this);
    video_sink_ = new QVideoSink(this);
    image_capture_ = new QImageCapture(this);
    media_recorder_ = new QMediaRecorder(this);

    connect(video_sink_, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        if (!frame.isValid()) {
            return;
        }
        QImage image = frame.toImage();
        if (!image.isNull()) {
            current_frame_ = image;
            update();
        }
    });

    capture_session_->setCamera(camera_);
    capture_session_->setVideoOutput(video_sink_);
    capture_session_->setImageCapture(image_capture_);
    capture_session_->setRecorder(media_recorder_);

    camera_->start();
}

int CameraWidget::currentCameraIndex() const
{
    return current_camera_index_;
}

void CameraWidget::restartCamera()
{
    if (media_recorder_ && media_recorder_->recorderState() == QMediaRecorder::RecordingState) {
        media_recorder_->stop();
    }

    if (camera_) {
        camera_->stop();
    }

    delete capture_session_;
    capture_session_ = nullptr;
    delete image_capture_;
    image_capture_ = nullptr;
    delete media_recorder_;
    media_recorder_ = nullptr;
    delete video_sink_;
    video_sink_ = nullptr;
    delete camera_;
    camera_ = nullptr;
    current_frame_ = QImage();

    initCamera();
}

void CameraWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void CameraWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (camera_ && camera_->isActive() && !current_frame_.isNull()) {
        const QImage frame = current_frame_.convertToFormat(QImage::Format_RGB32);
        const QSize scaledSize = frame.size().scaled(size(), Qt::KeepAspectRatioByExpanding);
        const QRect target((width() - scaledSize.width()) / 2,
                           (height() - scaledSize.height()) / 2,
                           scaledSize.width(), scaledSize.height());
        painter.drawImage(target, frame);
        return;
    }

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
