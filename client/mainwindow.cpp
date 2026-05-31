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
#include <memory>


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

    // 底部 4 个按钮的信号连接 (按钮本身的样式 / 文字 / checkable 等已在 .ui 中描述)
    connect(ui->micButton,     &QPushButton::toggled, this, &MainWindow::onMicrophoneToggled);
    connect(ui->sendButton,    &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(ui->videoButton,   &QPushButton::toggled, this, &MainWindow::onVideoToggled);
    connect(ui->endCallButton, &QPushButton::clicked, this, &MainWindow::onEndCallClicked);

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
        FaceManagerDialog dlg(retinaPath, arcfacePath, dbPath, this);
        dlg.exec();
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
    QString dbPath = appDir + QStringLiteral("/face.db");

    if (!QFile::exists(retinaPath) || !QFile::exists(arcfacePath)) {
        QMessageBox::warning(this, QStringLiteral("缺少模型"),
            QStringLiteral("请确保 models 目录存在"));
        return;
    }

    // 延迟初始化模型（只执行一次）
    static std::unique_ptr<FaceDetector> detector;
    static std::unique_ptr<FaceAligner> aligner;
    static std::unique_ptr<FaceRecognizer> recognizer;
    static std::unique_ptr<FaceDatabase> db;
    if (!detector) {
        try {
            detector = std::make_unique<FaceDetector>(retinaPath.toStdString());
            aligner = std::make_unique<FaceAligner>();
            recognizer = std::make_unique<FaceRecognizer>(arcfacePath.toStdString());
            db = std::make_unique<FaceDatabase>(dbPath);
        } catch (const std::exception& e) {
            QMessageBox::critical(this, QStringLiteral("错误"),
                QStringLiteral("模型加载失败: %1").arg(QString::fromLocal8Bit(e.what())));
            return;
        }
    }

    // 抓取一帧
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("无法打开摄像头"));
        return;
    }
    cv::Mat frame;
    cap >> frame;
    cap.release();
    if (frame.empty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("抓帧失败"));
        return;
    }

    // 检测人脸
    auto faces = detector->detect(frame);
    if (faces.empty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("未检测到人脸"));
        return;
    }

    // 对齐并提取特征
    cv::Mat aligned = aligner->align(frame, faces[0].landmarks);
    if (aligned.empty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("人脸对齐失败"));
        return;
    }
    std::vector<float> feature = recognizer->extract(aligned);

    // 与数据库比对（余弦相似度）
    auto records = db->getAllFaceFeatures();
    float best_sim = -1.0f;
    QString best_name;
    for (const auto& r : records) {
        if (r.feature.size() != feature.size()) continue;
        float dot = 0, na = 0, nb = 0;
        for (size_t i = 0; i < feature.size(); ++i) {
            dot += feature[i] * r.feature[i];
            na += feature[i] * feature[i];
            nb += r.feature[i] * r.feature[i];
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
        QString msg = QStringLiteral("识别到 %1 相似度 %2%")
                          .arg(best_name)
                          .arg(QString::number(best_sim * 100, 'f', 1));
#ifdef HAS_TEXTTOSPEECH
        QTextToSpeech tts;
        tts.say(msg);
#else
        QMessageBox::information(this, QStringLiteral("识别结果"), msg);
#endif
    } else {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("未匹配到已知人员"));
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
