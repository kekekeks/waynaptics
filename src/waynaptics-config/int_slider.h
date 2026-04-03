#pragma once

#include <QWidget>
#include <QSlider>
#include <QSpinBox>
#include <QHBoxLayout>

// A horizontal slider paired with a spinbox for integer values
class IntSlider : public QWidget
{
    Q_OBJECT

public:
    explicit IntSlider(int min, int max, QWidget *parent = nullptr);

    int value() const;
    void setValue(int v);

    void setRange(int min, int max);
    QSpinBox *spinBox() const { return m_spin; }

signals:
    void valueChanged(int value);

private:
    void onSliderChanged(int pos);
    void onSpinChanged(int val);

    QSlider *m_slider;
    QSpinBox *m_spin;
    bool m_updating = false;
};
