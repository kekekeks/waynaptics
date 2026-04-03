#pragma once

#include <QWidget>

class QTextEdit;
class TestButton;

class TestArea : public QWidget
{
    Q_OBJECT

public:
    explicit TestArea(QWidget *parent = nullptr);
};
