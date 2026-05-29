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

#include <QGraphicsDropShadowEffect>
#include <QDebug>


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
    // 按钮 2: 抓取摄像头快照 + 取已识别文本 "发送"
    if (ui->cameraWidget) {
        ui->cameraWidget->capturePhoto();
    }
    const QString pending_text = ui->transcriptionLabel->text().trimmed();
    qInfo() << "[Send] snapshot captured, transcription:" << pending_text;

    // 简单 UI 反馈
    ui->voiceHintLabel->setText(QStringLiteral("已发送"));
    QTimer::singleShot(1200, this, [this]() {
        ui->voiceHintLabel->setText(is_mic_active_ ? QStringLiteral("正在倾听...")
                                                   : QStringLiteral("你可以开始说话"));
    });
}

void MainWindow::onVideoToggled(bool checked)
{
    is_video_recording_ = checked;
    // 按钮 3 (视频) 的红色"录制中"视觉态已由 .ui 中的 QSS :checked 自动切换
    // 这里只切换图标文字与启停摄像头录制
    ui->videoButton->setText(checked ? QStringLiteral("●") : QStringLiteral("🎥"));

    if (!ui->cameraWidget) {
        return;
    }
    if (checked) {
        ui->cameraWidget->startRecording();
    } else {
        ui->cameraWidget->stopRecording();
    }
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
