#pragma once

#include <QDoubleSpinBox>

class E33DoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit E33DoubleSpinBox(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
};
