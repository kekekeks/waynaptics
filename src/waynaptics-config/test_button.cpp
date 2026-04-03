#include "test_button.h"
#include <QMouseEvent>
#include <QTimer>

TestButton::TestButton(QWidget *parent)
    : QPushButton(tr("Click me"), parent)
{
    setFocusPolicy(Qt::NoFocus);
}

void TestButton::mousePressEvent(QMouseEvent *e)
{
    if (m_originalText.isEmpty())
        m_originalText = text();

    switch (e->button()) {
    case Qt::LeftButton:
        setText(tr("Left button"));
        break;
    case Qt::RightButton:
        setText(tr("Right button"));
        break;
    case Qt::MiddleButton:
        setText(tr("Middle button"));
        break;
    default:
        setText(tr("Button %1").arg(static_cast<int>(e->button())));
        break;
    }

    QTimer::singleShot(500, this, &TestButton::resetText);
}

void TestButton::resetText()
{
    if (!m_originalText.isEmpty())
        setText(m_originalText);
}
