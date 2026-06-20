#pragma once
#include <QWidget>

class QDoubleSpinBox;

class EditCrop : public QWidget
{
    Q_OBJECT
public:
    explicit EditCrop(QWidget *parent = nullptr);

    /* Refrescar desde el modelo */
    void setCrop(double left, double top, double right, double bottom);

signals:
    void cropLeftChanged(double);
    void cropTopChanged(double);
    void cropRightChanged(double);
    void cropBottomChanged(double);

private:
    QDoubleSpinBox *spinL = nullptr;
    QDoubleSpinBox *spinT = nullptr;
    QDoubleSpinBox *spinR = nullptr;
    QDoubleSpinBox *spinB = nullptr;

    bool blockSignalsFlag = false;
};
