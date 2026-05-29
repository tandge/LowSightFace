/**
 * @file volumeslider.cpp
 * @brief Volume slider component implementation
 */

#include "volumeslider.h"
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>

VolumeSlider::VolumeSlider(QWidget *parent)
    : QWidget(parent)
    , slider_(nullptr)
    , icon_label_(nullptr)
    , value_label_(nullptr)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    icon_label_ = new QLabel("Speaker", this);
    icon_label_->setFixedSize(32, 32);
    icon_label_->setAlignment(Qt::AlignCenter);
    icon_label_->setStyleSheet(
        "color: #FFFFFF;"
        "font-size: 16px;"
        "font-weight: 500;"
        "background: rgba(255,255,255,0.15);"
        "border-radius: 16px;"
    );
    layout->addWidget(icon_label_);

    slider_ = new QSlider(Qt::Horizontal, this);
    slider_->setRange(0, 100);
    slider_->setValue(50);
    slider_->setFixedWidth(200);
    slider_->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  height: 6px;"
        "  background: rgba(255,255,255,0.3);"
        "  border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #FFFFFF;"
        "  width: 20px;"
        "  height: 20px;"
        "  margin: -7px 0;"
        "  border-radius: 10px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: rgba(255,255,255,0.8);"
        "  border-radius: 3px;"
        "}"
    );
    layout->addWidget(slider_);

    value_label_ = new QLabel("50", this);
    value_label_->setFixedWidth(30);
    value_label_->setAlignment(Qt::AlignCenter);
    value_label_->setStyleSheet(
        "color: #FFFFFF;"
        "font-size: 14px;"
        "font-weight: 500;"
        "background: rgba(255,255,255,0.15);"
        "border-radius: 4px;"
        "padding: 2px 4px;"
    );
    layout->addWidget(value_label_);

    connect(slider_, &QSlider::valueChanged, this, [this](int val) {
        value_label_->setText(QString::number(val));
        emit volumeChanged(val);
    });

    setStyleSheet("background: transparent;");
}

int VolumeSlider::volume() const
{
    return slider_ ? slider_->value() : 50;
}

void VolumeSlider::setVolume(int vol)
{
    if (slider_) {
        slider_->setValue(qBound(0, vol, 100));
    }
}
