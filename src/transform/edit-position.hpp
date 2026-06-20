#pragma once
#include <QWidget>

class QDoubleSpinBox;
class AlignmentGrid;

class EditPosition : public QWidget
{
    Q_OBJECT
public:
    explicit EditPosition(QWidget *parent = nullptr);

    /* Métodos para refrescar valores desde el modelo */
    void setPosition(double x, double y);
    void setSize(double w, double h);
    void setRotation(double rot);
    void setAlignment(int index);

signals:
    void posXChanged(double);
    void posYChanged(double);

    void widthChanged(double);
    void heightChanged(double);

    void rotationChanged(double);

    void alignmentChanged(int);

private:
    /* Guardamos punteros a los widgets */
    QDoubleSpinBox *spinX = nullptr;
    QDoubleSpinBox *spinY = nullptr;

    QDoubleSpinBox *spinW = nullptr;
    QDoubleSpinBox *spinH = nullptr;

    QDoubleSpinBox *spinRot = nullptr;

    AlignmentGrid *grid = nullptr;

    bool blockSignalsFlag = false;
};
