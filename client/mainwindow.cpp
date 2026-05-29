/**
 * @file mainwindow.cpp
 * @brief Main window implementation
 *
 * Follows gui_spec.txt design spec:
 * - 375px width (adaptive in WASM)
 * - 85vh height
 * - 24px border-radius phone frame
 * - Linear gradient bg #F0F0F0 -> #E8E8E8
 */

#include "mainwindow.h"
#include "camerawidget.h"
#include "pulsedots.h"
#include "volumeslider.h"
#include "whisperclient.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QScreen>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , camera_widget_(nullptr)
    , time_label_(nullptr)
    , pulse_dots_(nullptr)
    , voice_hint_label_(nullptr)
    , transcription_label_(nullptr)
    , volume_slider_(nullptr)
    , mic_button_(nullptr)
    , photo_button_(nullptr)
    , video_button_(nullptr)
    , exit_button_(nullptr)
    , whisper_client_(nullptr)
    , is_mic_active_(false)
    , is_video_recording_(false)
    , clock_timer_(nullptr)
{
    setWindowTitle("BntechEyeFriend");
    setAttribute(Qt::WA_TranslucentBackground, false);

    const int canvas_width = 375;
    const int canvas_height = 680;
    setFixedSize(canvas_width, canvas_height);

    setStyleSheet(
        "MainWindow {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #F0F0F0, stop:1 #E8E8E8);"
        "  border-radius: 24px;"
        "}"
    );

    QGraphicsDropShadowEffect *window_shadow = new QGraphicsDropShadowEffect(this);
    window_shadow->setBlurRadius(40);
    window_shadow->setOffset(0, 8);
    window_shadow->setColor(QColor(0, 0, 0, 60));
    setGraphicsEffect(window_shadow);

    whisper_client_ = new WhisperClient(this);
    connect(whisper_client_, &WhisperClient::transcriptionReady,
            this, &MainWindow::onTranscriptionReceived);
    connect(whisper_client_, &WhisperClient::errorOccurred,
            this, [this](const QString &error) {
        qWarning() << "Whisper error:" << error;
    });

    initUi();
    applyStyles();

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
}

void MainWindow::initUi()
{
    QWidget *central_widget = new QWidget(this);
    setCentralWidget(central_widget);
    central_widget->setStyleSheet("background: transparent;");

    camera_widget_ = new CameraWidget(central_widget);
    camera_widget_->setGeometry(0, 0, width(), height());
    camera_widget_->lower();

    QWidget *overlay = new QWidget(central_widget);
    overlay->setGeometry(0, 0, width(), height());
    overlay->setStyleSheet("background: transparent;");
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    setupStatusBar();
    setupTopActionButtons();
    setupCenterArea();
    setupBottomButtons();
    setupCopyright();
}

void MainWindow::setupStatusBar()
{
    QWidget *status_bar = new QWidget(centralWidget());
    status_bar->setGeometry(0, 0, width(), 56);
    status_bar->setStyleSheet("background: transparent;");

    QHBoxLayout *status_layout = new QHBoxLayout(status_bar);
    status_layout->setContentsMargins(24, 16, 24, 4);

    time_label_ = new QLabel("15:09", status_bar);
    time_label_->setStyleSheet(
        "color: #FFFFFF;"
        "font-family: 'Inter', -apple-system, sans-serif;"
        "font-size: 24px;"
        "font-weight: 500;"
    );
    QGraphicsDropShadowEffect *text_shadow = new QGraphicsDropShadowEffect(time_label_);
    text_shadow->setBlurRadius(4);
    text_shadow->setOffset(0, 1);
    text_shadow->setColor(QColor(0, 0, 0, 80));
    time_label_->setGraphicsEffect(text_shadow);

    status_layout->addWidget(time_label_);
    status_layout->addStretch();

    QLabel *signal_icon = new QLabel(status_bar);
    signal_icon->setFixedSize(18, 18);
    signal_icon->setStyleSheet(
        "background: qlineargradient(x1:0, y1:1, x2:0, y2:0,"
        " stop:0 rgba(255,255,255,0.3), stop:0.3 rgba(255,255,255,0.5),"
        " stop:0.6 rgba(255,255,255,0.8), stop:1 #FFFFFF);"
        "border-radius: 2px;"
    );
    status_layout->addWidget(signal_icon);
    status_layout->addSpacing(12);

    QLabel *wifi_icon = new QLabel(status_bar);
    wifi_icon->setFixedSize(18, 18);
    wifi_icon->setStyleSheet("background: #FFFFFF; border-radius: 9px;");
    status_layout->addWidget(wifi_icon);
    status_layout->addSpacing(12);

    QLabel *battery_label = new QLabel("62", status_bar);
    battery_label->setStyleSheet(
        "color: #000000;"
        "background: rgba(255,255,255,0.8);"
        "font-size: 14px;"
        "font-weight: 500;"
        "padding: 2px 8px;"
        "border-radius: 4px;"
    );
    status_layout->addWidget(battery_label);
}

void MainWindow::setupTopActionButtons()
{
    QWidget *action_bar = new QWidget(centralWidget());
    action_bar->setGeometry(width() - 80, 64, 56, 140);
    action_bar->setStyleSheet("background: transparent;");

    QVBoxLayout *action_layout = new QVBoxLayout(action_bar);
    action_layout->setContentsMargins(0, 0, 0, 0);
    action_layout->setSpacing(24);

    auto createTopButton = [&](const QString &text, const QString &tooltip) -> QPushButton* {
        QPushButton *btn = new QPushButton(text, action_bar);
        btn->setFixedSize(28, 28);
        btn->setToolTip(tooltip);
        btn->setStyleSheet(
            "QPushButton {"
            "  background: transparent;"
            "  color: #FFFFFF;"
            "  font-size: 18px;"
            "  font-weight: bold;"
            "  border: none;"
            "}"
            "QPushButton:hover {"
            "  background: rgba(255,255,255,0.2);"
            "  border-radius: 6px;"
            "}"
        );
        return btn;
    };

    QPushButton *filter_btn = createTopButton("F", "Filter");
    QPushButton *rotate_btn = createTopButton("R", "Rotate");
    QPushButton *text_btn   = createTopButton("T", "Text");

    action_layout->addWidget(filter_btn);
    action_layout->addWidget(rotate_btn);
    action_layout->addWidget(text_btn);
    action_layout->addStretch();
}

void MainWindow::setupCenterArea()
{
    QWidget *center_area = new QWidget(centralWidget());
    int center_y = height() / 2 - 100;
    center_area->setGeometry(0, center_y, width(), 120);
    center_area->setStyleSheet("background: transparent;");

    QVBoxLayout *center_layout = new QVBoxLayout(center_area);
    center_layout->setAlignment(Qt::AlignCenter);
    center_layout->setSpacing(16);

    pulse_dots_ = new PulseDots(center_area);
    center_layout->addWidget(pulse_dots_, 0, Qt::AlignCenter);

    voice_hint_label_ = new QLabel("You can start speaking", center_area);
    voice_hint_label_->setAlignment(Qt::AlignCenter);
    voice_hint_label_->setStyleSheet(
        "color: #FFFFFF;"
        "font-family: 'Inter', -apple-system, sans-serif;"
        "font-size: 20px;"
        "font-weight: 500;"
    );
    QGraphicsDropShadowEffect *hint_shadow = new QGraphicsDropShadowEffect(voice_hint_label_);
    hint_shadow->setBlurRadius(6);
    hint_shadow->setOffset(0, 2);
    hint_shadow->setColor(QColor(0, 0, 0, 100));
    voice_hint_label_->setGraphicsEffect(hint_shadow);
    center_layout->addWidget(voice_hint_label_);

    transcription_label_ = new QLabel(centralWidget());
    transcription_label_->setGeometry(24, height() / 2 + 40, width() - 48, 60);
    transcription_label_->setAlignment(Qt::AlignCenter);
    transcription_label_->setWordWrap(true);
    transcription_label_->setStyleSheet(
        "color: #FFFFFF;"
        "font-family: 'Inter', -apple-system, sans-serif;"
        "font-size: 16px;"
        "font-weight: 400;"
        "background: rgba(0,0,0,0.35);"
        "border-radius: 12px;"
        "padding: 8px 12px;"
    );
    transcription_label_->hide();
}

void MainWindow::setupBottomButtons()
{
    QWidget *bottom_area = new QWidget(centralWidget());
    int bottom_y = height() - 200;
    bottom_area->setGeometry(0, bottom_y, width(), 180);
    bottom_area->setStyleSheet("background: transparent;");

    QVBoxLayout *bottom_layout = new QVBoxLayout(bottom_area);
    bottom_layout->setContentsMargins(16, 0, 16, 0);
    bottom_layout->setSpacing(16);

    volume_slider_ = new VolumeSlider(bottom_area);
    bottom_layout->addWidget(volume_slider_, 0, Qt::AlignCenter);

    QHBoxLayout *button_row = new QHBoxLayout();
    button_row->setSpacing(24);
    button_row->setAlignment(Qt::AlignCenter);

    auto createRoundButton = [&](const QString &text,
                                  const QString &bg,
                                  const QString &color,
                                  int font_size) -> QPushButton* {
        QPushButton *btn = new QPushButton(text, bottom_area);
        btn->setFixedSize(96, 96);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            QString(
                "QPushButton {"
                "  background: %1;"
                "  color: %2;"
                "  font-size: %3px;"
                "  font-weight: bold;"
                "  border: none;"
                "  border-radius: 48px;"
                "}"
                "QPushButton:hover {"
                "  transform: scale(1.05);"
                "}"
            ).arg(bg, color).arg(font_size)
        );
        QGraphicsDropShadowEffect *btn_shadow = new QGraphicsDropShadowEffect(btn);
        btn_shadow->setBlurRadius(8);
        btn_shadow->setOffset(0, 2);
        btn_shadow->setColor(QColor(0, 0, 0, 38));
        btn->setGraphicsEffect(btn_shadow);
        return btn;
    };

    mic_button_ = createRoundButton("Mic", "rgba(255,255,255,0.90)", "#444444", 24);
    mic_button_->setCheckable(true);
    mic_button_->setToolTip("Microphone - Voice Interaction");
    connect(mic_button_, &QPushButton::toggled, this, &MainWindow::onMicrophoneToggled);

    photo_button_ = createRoundButton("Photo", "rgba(255,255,255,0.70)", "#555555", 18);
    photo_button_->setToolTip("Take Photo");
    connect(photo_button_, &QPushButton::clicked, this, &MainWindow::onPhotoClicked);

    video_button_ = createRoundButton("Video", "#FFFFFF", "#333333", 16);
    video_button_->setCheckable(true);
    video_button_->setToolTip("Video Recording");
    connect(video_button_, &QPushButton::toggled, this, &MainWindow::onVideoToggled);

    exit_button_ = createRoundButton("X", "rgba(255,255,255,0.90)", "#FF5252", 32);
    exit_button_->setToolTip("Exit");
    connect(exit_button_, &QPushButton::clicked, this, &MainWindow::onExitClicked);

    button_row->addWidget(mic_button_);
    button_row->addWidget(photo_button_);
    button_row->addWidget(video_button_);
    button_row->addWidget(exit_button_);

    bottom_layout->addLayout(button_row);

    connect(volume_slider_, &VolumeSlider::volumeChanged,
            this, &MainWindow::onVolumeChanged);
}

void MainWindow::setupCopyright()
{
    QLabel *copyright = new QLabel("Content generated by AI", centralWidget());
    copyright->setGeometry(0, height() - 32, width(), 20);
    copyright->setAlignment(Qt::AlignCenter);
    copyright->setStyleSheet(
        "color: rgba(255,255,255,0.70);"
        "font-family: 'Inter', -apple-system, sans-serif;"
        "font-size: 12px;"
        "font-weight: 400;"
    );
}

void MainWindow::applyStyles()
{
    QFont global_font("Inter", 10);
    global_font.setStyleStrategy(QFont::PreferAntialias);
    QApplication::setFont(global_font);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (camera_widget_) {
        camera_widget_->setGeometry(0, 0, width(), height());
    }
}

// ============================================================
// Slot Implementations
// ============================================================

void MainWindow::onMicrophoneToggled(bool checked)
{
    is_mic_active_ = checked;
    updateMicrophoneButtonStyle(checked);

    if (checked) {
        pulse_dots_->startAnimation();
        voice_hint_label_->setText(QStringLiteral("Listening..."));
        transcription_label_->show();
        whisper_client_->startRecognition();
    } else {
        pulse_dots_->stopAnimation();
        voice_hint_label_->setText(QStringLiteral("You can start speaking"));
        whisper_client_->stopRecognition();

        QTimer::singleShot(2000, this, [this]() {
            if (!is_mic_active_) {
                transcription_label_->hide();
                transcription_label_->clear();
            }
        });
    }
}

void MainWindow::onPhotoClicked()
{
    if (camera_widget_) {
        camera_widget_->capturePhoto();
    }

    photo_button_->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: rgba(255,255,255,0.95);"
        "  color: #555555;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  border: none;"
        "  border-radius: 48px;"
        "}"
    ));
    QTimer::singleShot(200, this, [this]() {
        photo_button_->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: rgba(255,255,255,0.70);"
            "  color: #555555;"
            "  font-size: 18px;"
            "  font-weight: bold;"
            "  border: none;"
            "  border-radius: 48px;"
            "}"
            "QPushButton:hover { transform: scale(1.05); }"
        ));
    });
}

void MainWindow::onVideoToggled(bool checked)
{
    is_video_recording_ = checked;

    if (checked) {
        video_button_->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: #FF5252;"
            "  color: #FFFFFF;"
            "  font-size: 16px;"
            "  font-weight: bold;"
            "  border: none;"
            "  border-radius: 48px;"
            "}"
            "QPushButton:hover { transform: scale(1.05); }"
        ));
        video_button_->setText(QStringLiteral("REC"));
        if (camera_widget_) {
            camera_widget_->startRecording();
        }
    } else {
        video_button_->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: #FFFFFF;"
            "  color: #333333;"
            "  font-size: 16px;"
            "  font-weight: bold;"
            "  border: none;"
            "  border-radius: 48px;"
            "}"
            "QPushButton:hover { transform: scale(1.05); }"
        ));
        video_button_->setText(QStringLiteral("Video"));
        if (camera_widget_) {
            camera_widget_->stopRecording();
        }
    }
}

void MainWindow::onExitClicked()
{
    if (is_mic_active_) {
        whisper_client_->stopRecognition();
        pulse_dots_->stopAnimation();
    }
    if (is_video_recording_ && camera_widget_) {
        camera_widget_->stopRecording();
    }
    close();
}

void MainWindow::updateStatusBarTime()
{
    QString current_time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"));
    time_label_->setText(current_time);
}

void MainWindow::onVolumeChanged(int volume)
{
    if (camera_widget_) {
        camera_widget_->setVolume(volume);
    }
    if (whisper_client_) {
        whisper_client_->setVolume(volume);
    }
}

void MainWindow::onTranscriptionReceived(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        return;
    }
    transcription_label_->setText(text);
    transcription_label_->show();

    QTimer::singleShot(3000, this, [this, text]() {
        if (transcription_label_->text() == text) {
            transcription_label_->clear();
            if (!is_mic_active_) {
                transcription_label_->hide();
            }
        }
    });
}

void MainWindow::updateMicrophoneButtonStyle(bool active)
{
    if (active) {
        mic_button_->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: #4CAF50;"
            "  color: #FFFFFF;"
            "  font-size: 24px;"
            "  font-weight: bold;"
            "  border: none;"
            "  border-radius: 48px;"
            "}"
            "QPushButton:hover { transform: scale(1.05); }"
        ));
    } else {
        mic_button_->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: rgba(255,255,255,0.90);"
            "  color: #444444;"
            "  font-size: 24px;"
            "  font-weight: bold;"
            "  border: none;"
            "  border-radius: 48px;"
            "}"
            "QPushButton:hover { transform: scale(1.05); }"
        ));
    }
}
