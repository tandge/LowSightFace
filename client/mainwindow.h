/**
 * @file mainwindow.h
 * @brief 主窗口 - 应用顶层容器
 *
 * 主窗口负责整合所有子组件：
 * - CameraWidget: 摄像头实时画面背景
 * - PulseDots: 语音状态脉冲圆点动画
 * - VolumeSlider: 音量调节滑动条
 * - 底部4个功能按钮: 麦克风/拍照/视频/退出
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QDateTime>

class CameraWidget;
class PulseDots;
class VolumeSlider;
class WhisperClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onMicrophoneToggled(bool checked);
    void onPhotoClicked();
    void onVideoToggled(bool checked);
    void onExitClicked();
    void updateStatusBarTime();
    void onVolumeChanged(int volume);
    void onTranscriptionReceived(const QString &text);

private:
    void initUi();
    void setupStatusBar();
    void setupTopActionButtons();
    void setupCenterArea();
    void setupBottomButtons();
    void setupCopyright();
    void applyStyles();
    void updateMicrophoneButtonStyle(bool active);

    // 摄像头实时画面
    CameraWidget *camera_widget_;

    // 顶部状态栏
    QLabel *time_label_;

    // 语音状态区
    PulseDots *pulse_dots_;
    QLabel *voice_hint_label_;
    QLabel *transcription_label_;  // 识别文字输出

    // 音量滑动条
    VolumeSlider *volume_slider_;

    // 底部功能按钮
    QPushButton *mic_button_;
    QPushButton *photo_button_;
    QPushButton *video_button_;
    QPushButton *exit_button_;

    // 语音识别客户端
    WhisperClient *whisper_client_;

    // 状态标记
    bool is_mic_active_;
    bool is_video_recording_;

    // 时间刷新定时器
    QTimer *clock_timer_;
};

#endif // MAINWINDOW_H
