/**
 * @file mainwindow.cpp
 * @brief Main window implementation
 *
 * UI 由 mainwindow.ui 描述, uic 自动生成 ui_mainwindow.h, 本文件仅
 * 处理业务逻辑 (按钮槽函数 / 状态切换 / 摄像头与语音识别交互)。
 *
 * 视觉规范参考 gui_spec.txt / docs/gui.html:
 * - 375 x 680 画布, 24px 圆角
 * - 线性渐变背景 #F0F0F0 -> #E8E8E8
 * - 底部 4 个圆形按钮 (麦克风 / 发送 / 视频 / 结束通话)
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "camerawidget.h"
#include "pulsedots.h"
#include "whisperclient.h"

#ifndef Q_OS_WASM
#include "facemanagerdialog.h"
#include "facedetector.h"
#include "facealigner.h"
#include "facerecognizer.h"
#include "facedatabase.h"
#ifdef HAS_TEXTTOSPEECH
#include <QTextToSpeech>
#endif
#include <opencv2/opencv.hpp>
#endif

#include <QGraphicsDropShadowEffect>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>
#include <QCameraInfo>
#include <memory>

#ifndef Q_OS_WASM
namespace {
    std::unique_ptr<FaceDetector>   g_detector;
    std::unique_ptr<FaceAligner>    g_aligner;
    std::unique_ptr<FaceRecognizer> g_recognizer;
    std::unique_ptr<FaceDatabase>   g_db;
}
#endif


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , whisper_client_(nullptr)
    , is_mic_active_(false)
    , is_video_recording_(false)
    , is_call_active_(true)
    , clock_timer_(nullptr)
{
    // 由 uic 生成的 setupUi() 一次性构造全部子控件 / 布局 / 样式
    ui->setupUi(this);

    // 视觉装饰: 窗口阴影 + 时间文字阴影 (这类 QGraphicsEffect 不便在 .ui 内描述)
    auto *window_shadow = new QGraphicsDropShadowEffect(this);
    window_shadow->setBlurRadius(40);
    window_shadow->setOffset(0, 8);
    window_shadow->setColor(QColor(0, 0, 0, 60));
    setGraphicsEffect(window_shadow);

    auto *time_shadow = new QGraphicsDropShadowEffect(ui->timeLabel);
    time_shadow->setBlurRadius(4);
    time_shadow->setOffset(0, 1);
    time_shadow->setColor(QColor(0, 0, 0, 80));
    ui->timeLabel->setGraphicsEffect(time_shadow);

    auto *hint_shadow = new QGraphicsDropShadowEffect(ui->voiceHintLabel);
    hint_shadow->setBlurRadius(6);
    hint_shadow->setOffset(0, 2);
    hint_shadow->setColor(QColor(0, 0, 0, 100));
    ui->voiceHintLabel->setGraphicsEffect(hint_shadow);

    // 语音识别后端
    whisper_client_ = new WhisperClient(this);
    connect(whisper_client_, &WhisperClient::transcriptionReady,
            this, &MainWindow::onTranscriptionReceived);
    connect(whisper_client_, &WhisperClient::errorOccurred, this,
            [](const QString &error) { qWarning() << "Whisper error:" << error; });

#ifdef HAS_TEXTTOSPEECH
    // 语音合成后端 (成员变量, 避免局部对象被提前销毁)
    tts_ = new QTextToSpeech(this);
#endif

    // 实时视频流帧捕获 -> 人脸识别
#ifndef Q_OS_WASM
    if (ui->cameraWidget) {
        connect(ui->cameraWidget, &CameraWidget::frameCaptured,
                this, &MainWindow::onFrameCapturedForFace);
    }
#endif

    // 底部 4 个按钮的信号连接 (按钮本身的样式 / 文字 / checkable 等已在 .ui 中描述)
    connect(ui->micButton,     &QPushButton::toggled, this, &MainWindow::onMicrophoneToggled);
    connect(ui->sendButton,    &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(ui->videoButton,   &QPushButton::toggled, this, &MainWindow::onVideoToggled);
    connect(ui->endCallButton, &QPushButton::clicked, this, &MainWindow::onEndCallClicked);

    // 枚举摄像头设备并填充顶部下拉框
    if (ui->cameraComboBox) {
        QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
        for (const QCameraInfo &info : cameras) {
            QString label = info.description();
            if (info.position() == QCamera::FrontFace)
                label += QStringLiteral(" [前置]");
            else if (info.position() == QCamera::BackFace)
                label += QStringLiteral(" [后置]");
            ui->cameraComboBox->addItem(label);
        }
        if (cameras.size() <= 1) {
            ui->cameraComboBox->setVisible(false);
        } else if (ui->cameraWidget) {
            int idx = ui->cameraWidget->currentCameraIndex();
            if (idx >= 0 && idx < cameras.size()) {
                ui->cameraComboBox->setCurrentIndex(idx);
            }
            connect(ui->cameraComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int index) {
                if (ui->cameraWidget) {
                    ui->cameraWidget->setCameraDevice(index);
                }
            });
        }
    }

    // 状态栏时间刷新
    clock_timer_ = new QTimer(this);
    connect(clock_timer_, &QTimer::timeout, this, &MainWindow::updateStatusBarTime);
    clock_timer_->start(1000);
    updateStatusBarTime();
}

MainWindow::~MainWindow()
{
    if (whisper_client_) {
        whisper_client_->stopRecognition();
    }
    delete ui;
}

// ============================================================
// 槽函数 - 与 docs/gui.html 底部四个按钮一一对应
// ============================================================

void MainWindow::onMicrophoneToggled(bool checked)
{
    is_mic_active_ = checked;
    // 按钮 1 (麦克风) 的激活态视觉切换已由 .ui 中的 QSS :checked 选择器自动完成

    if (checked) {
#ifdef HAS_TEXTTOSPEECH
        if (tts_) {
            tts_->say(QStringLiteral("打开麦克风"));
        }
#endif
        ui->pulseDots->startAnimation();
        ui->voiceHintLabel->setText(QStringLiteral("正在倾听..."));
        ui->transcriptionLabel->show();
        whisper_client_->startRecognition();
    } else {
        ui->pulseDots->stopAnimation();
        ui->voiceHintLabel->setText(QStringLiteral("你可以开始说话"));
        whisper_client_->stopRecognition();

        QTimer::singleShot(2000, this, [this]() {
            if (!is_mic_active_) {
                ui->transcriptionLabel->hide();
                ui->transcriptionLabel->clear();
            }
        });
    }
}

void MainWindow::onSendClicked()
{
#ifndef Q_OS_WASM
    QString appDir = QCoreApplication::applicationDirPath();
    QString retinaPath = appDir + QStringLiteral("/models/retinaface.onnx");
    QString arcfacePath  = appDir + QStringLiteral("/models/arcface_r50.onnx");
    QString dbPath       = appDir + QStringLiteral("/face.db");

    if (!QFile::exists(retinaPath)) {
        QMessageBox::warning(this, QStringLiteral("缺少模型"),
            QStringLiteral("未找到检测模型:\n%1\n请将 retinaface.onnx 放到 exe 同级的 models/ 目录下。").arg(retinaPath));
        return;
    }
    if (!QFile::exists(arcfacePath)) {
        QMessageBox::warning(this, QStringLiteral("缺少模型"),
            QStringLiteral("未找到识别模型:\n%1\n请将 arcface_r50.onnx 放到 exe 同级的 models/ 目录下。").arg(arcfacePath));
        return;
    }

    try {
        int camIdx = ui->cameraWidget ? ui->cameraWidget->currentCameraIndex() : 0;
        FaceManagerDialog dlg(retinaPath, arcfacePath, dbPath, camIdx, this);
        dlg.exec();

        // 人脸管理窗口也占用了摄像头，关闭后重启主窗口摄像头恢复实时画面
        if (ui->cameraWidget) {
            ui->cameraWidget->restartCamera();
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, QStringLiteral("人脸管理启动失败"),
            QStringLiteral("加载模型或初始化失败:\n%1").arg(QString::fromLocal8Bit(e.what())));
    }
#else
    ui->voiceHintLabel->setText(QStringLiteral("人脸管理不支持 Web 版本"));
    QTimer::singleShot(2000, this, [this]() {
        ui->voiceHintLabel->setText(is_mic_active_ ? QStringLiteral("正在倾听...")
                                                   : QStringLiteral("你可以开始说话"));
    });
#endif
}

void MainWindow::onVideoToggled(bool checked)
{
#ifndef Q_OS_WASM
    if (!checked) return;

    // 单次触发：立即恢复按钮状态，避免录制逻辑
    ui->videoButton->setChecked(false);

    QString appDir = QCoreApplication::applicationDirPath();
    QString retinaPath = appDir + QStringLiteral("/models/retinaface.onnx");
    QString arcfacePath = appDir + QStringLiteral("/models/arcface_r50.onnx");

    if (!QFile::exists(retinaPath) || !QFile::exists(arcfacePath)) {
        QMessageBox::warning(this, QStringLiteral("缺少模型"),
            QStringLiteral("请确保 models 目录存在"));
        return;
    }

    // 延迟初始化模型（只执行一次）
    if (!g_detector) {
        try {
            g_detector   = std::make_unique<FaceDetector>(retinaPath.toStdString());
            g_aligner    = std::make_unique<FaceAligner>();
            g_recognizer = std::make_unique<FaceRecognizer>(arcfacePath.toStdString());
            g_db         = std::make_unique<FaceDatabase>(appDir + QStringLiteral("/face.db"));
        } catch (const std::exception& e) {
            QMessageBox::critical(this, QStringLiteral("错误"),
                QStringLiteral("模型加载失败: %1").arg(QString::fromLocal8Bit(e.what())));
            return;
        }
    }

    // 向 CameraWidget 请求实时流的一帧（异步，结果通过 frameCaptured 信号返回）
    if (ui->cameraWidget) {
        ui->cameraWidget->requestFrame();
    }
#else
    Q_UNUSED(checked)
#endif
}

void MainWindow::onEndCallClicked()
{
    // 按钮 4: 红色 X - 结束通话, 停止全部任务并关闭窗口
    is_call_active_ = false;

    if (is_mic_active_) {
        whisper_client_->stopRecognition();
        ui->pulseDots->stopAnimation();
        is_mic_active_ = false;
        ui->micButton->blockSignals(true);
        ui->micButton->setChecked(false);
        ui->micButton->blockSignals(false);
    }
    if (is_video_recording_) {
        if (ui->cameraWidget) {
            ui->cameraWidget->stopRecording();
        }
        is_video_recording_ = false;
        ui->videoButton->blockSignals(true);
        ui->videoButton->setChecked(false);
        ui->videoButton->blockSignals(false);
        ui->videoButton->setText(QStringLiteral("🎥"));
    }

    ui->voiceHintLabel->setText(QStringLiteral("通话已结束"));
    QTimer::singleShot(400, this, &QWidget::close);
}

void MainWindow::updateStatusBarTime()
{
    ui->timeLabel->setText(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm")));
}

void MainWindow::onTranscriptionReceived(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        return;
    }
    ui->transcriptionLabel->setText(text);
    ui->transcriptionLabel->show();

    QTimer::singleShot(3000, this, [this, text]() {
        if (ui->transcriptionLabel->text() == text) {
            ui->transcriptionLabel->clear();
            if (!is_mic_active_) {
                ui->transcriptionLabel->hide();
            }
        }
    });
}

#ifndef Q_OS_WASM
void MainWindow::onFrameCapturedForFace(const QImage &frame)
{
    if (frame.isNull()) {
        qWarning() << "Captured frame is null";
        return;
    }

    // QImage -> cv::Mat (BGR)
    cv::Mat mat;
    switch (frame.format()) {
        case QImage::Format_RGB888:
            mat = cv::Mat(frame.height(), frame.width(), CV_8UC3,
                          const_cast<uchar*>(frame.bits()), frame.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
            break;
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
        case QImage::Format_RGB32:
            mat = cv::Mat(frame.height(), frame.width(), CV_8UC4,
                          const_cast<uchar*>(frame.bits()), frame.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
            break;
        default:
            qWarning() << "Unsupported QImage format:" << frame.format();
#ifdef HAS_TEXTTOSPEECH
            if (tts_) {
                tts_->say(QStringLiteral("图像格式不支持"));
            }
#endif
            return;
    }

    if (!g_detector || !g_aligner || !g_recognizer || !g_db) {
        qWarning() << "Face models not initialized";
        return;
    }

    // 检测人脸（可能多个）
    auto faces = g_detector->detect(mat);
    if (faces.empty()) {
#ifdef HAS_TEXTTOSPEECH
        if (tts_) {
            tts_->say(QStringLiteral("未检测到人脸"));
        }
#else
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("未检测到人脸"));
#endif
        return;
    }

    // 预加载数据库，避免循环内重复查询
    auto records = g_db->getAllFaceFeatures();
    QStringList recognizedNames;
    int unknownCount = 0;

    // 逐个识别（不播报，只收集结果）
    for (size_t i = 0; i < faces.size(); ++i) {
        cv::Mat aligned = g_aligner->align(mat, faces[i].landmarks);
        if (aligned.empty()) {
            ++unknownCount;
            continue;
        }

        std::vector<float> feature = g_recognizer->extract(aligned);

        // 余弦相似度比对
        float best_sim = -1.0f;
        QString best_name;
        for (const auto &r : records) {
            if (r.feature.size() != feature.size()) continue;
            float dot = 0, na = 0, nb = 0;
            for (size_t j = 0; j < feature.size(); ++j) {
                dot += feature[j] * r.feature[j];
                na += feature[j] * feature[j];
                nb += r.feature[j] * r.feature[j];
            }
            if (na > 0 && nb > 0) {
                float sim = dot / (std::sqrt(na) * std::sqrt(nb));
                if (sim > best_sim) {
                    best_sim = sim;
                    best_name = r.person_name;
                }
            }
        }

        if (best_sim >= 0.70f) {
            recognizedNames.append(best_name);
        } else {
            ++unknownCount;
        }
    }

    // 拼接成一句话，只播报一次
    QString finalMsg;
    int total = static_cast<int>(faces.size());
    if (!recognizedNames.isEmpty() && unknownCount == 0) {
        finalMsg = QStringLiteral("识别到 %1，共 %2 人")
                       .arg(recognizedNames.join("、"))
                       .arg(total);
    } else if (!recognizedNames.isEmpty() && unknownCount > 0) {
        finalMsg = QStringLiteral("识别到 %1，另有 %2 人未识别，共 %3 人")
                       .arg(recognizedNames.join("、"))
                       .arg(unknownCount)
                       .arg(total);
    } else {
        finalMsg = QStringLiteral("检测到 %1 人，未识别出已知人员").arg(total);
    }

#ifdef HAS_TEXTTOSPEECH
    if (tts_) {
        tts_->say(finalMsg);
    }
#else
    QMessageBox::information(this, QStringLiteral("识别结果"), finalMsg);
#endif
}
#endif
