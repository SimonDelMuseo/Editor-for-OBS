#pragma once
#include <QWidget>

class QuickBox : public QWidget
{
    Q_OBJECT
public:
    explicit QuickBox(QWidget *parent = nullptr);

signals:
    void flipHorizontal();
    void flipVertical();

    void rotate90CW();
    void rotate90CCW();
    void rotate180();
};
