#pragma once

#include <QPushButton>
#include <QString>

// Button that shows which mouse button was clicked, then resets after a timeout
class TestButton : public QPushButton
{
    Q_OBJECT

public:
    explicit TestButton(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *e) override;

private:
    void resetText();
    QString m_originalText;
};
