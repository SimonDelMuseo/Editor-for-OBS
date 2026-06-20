#pragma once
#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QCheckBox;
class AlignmentGrid;

class EditBounds : public QWidget
{
    Q_OBJECT
public:
    explicit EditBounds(QWidget *parent = nullptr);

    /* Métodos para refrescar desde el modelo */
    void setBoundsType(int index);
    void setBoundsSize(double w, double h);
    void setCropToBounds(bool enabled);
    void setAlignment(int index);

signals:
    void boundsTypeChanged(int index);
    void boundsWidthChanged(double);
    void boundsHeightChanged(double);
    void cropToBoundsChanged(bool);
    void alignmentChanged(int);

private:
    QComboBox      *comboType = nullptr;
    QDoubleSpinBox *spinW     = nullptr;
    QDoubleSpinBox *spinH     = nullptr;
    QCheckBox      *chkCrop   = nullptr;
    AlignmentGrid  *grid      = nullptr;

    bool blockSignalsFlag = false;
};
