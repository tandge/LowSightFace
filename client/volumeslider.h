/**
 * @file volumeslider.h
 * @brief Volume slider component
 *
 * Custom volume control slider positioned above the bottom buttons.
 * Controls camera audio volume and whisper input volume.
 */

#ifndef VOLUMESLIDER_H
#define VOLUMESLIDER_H

#include <QWidget>
#include <QSlider>
#include <QLabel>

class VolumeSlider : public QWidget
{
    Q_OBJECT

public:
    explicit VolumeSlider(QWidget *parent = nullptr);

    int volume() const;
    void setVolume(int vol);

signals:
    void volumeChanged(int volume);

private:
    QSlider *slider_;
    QLabel *icon_label_;
    QLabel *value_label_;
};

#endif // VOLUMESLIDER_H
