#pragma once
#include <QWidget>

class QDoubleSpinBox;
class AlignmentSelector;

class EditPosition : public QWidget
{
    Q_OBJECT
public:
    explicit EditPosition(QWidget *parent = nullptr);

    void setPosition(double x, double y);
    void setSize(double w, double h);
    void setRotation(double rot);
    void setAlignment(int index);
    void setSizesEnabled(bool enabled);

signals:
    void posXChanged(double);
    void posYChanged(double);

    void widthChanged(double);
    void heightChanged(double);

    void rotationChanged(double);

    void alignmentChanged(int);

private:
    QDoubleSpinBox *spinX = nullptr;
    QDoubleSpinBox *spinY = nullptr;

    QDoubleSpinBox *spinW = nullptr;
    QDoubleSpinBox *spinH = nullptr;

    QDoubleSpinBox *spinRot = nullptr;

    AlignmentSelector *grid = nullptr;
};
