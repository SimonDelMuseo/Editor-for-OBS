#pragma once
#include <QWidget>

class AlignmentBox : public QWidget
{
    Q_OBJECT
public:
    explicit AlignmentBox(QWidget *parent = nullptr);

signals:
    void centerVertical();
    void centerHorizontal();

    void fitToScreen();
    void stretchToScreen();
    void centerToScreen();
};
