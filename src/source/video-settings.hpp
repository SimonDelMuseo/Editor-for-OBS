#pragma once
#include <QWidget>
#include "src/widgets/box.hpp"

class QComboBox;
class QRadioButton;
class EditorModel;

class VideoSettings : public Box
{
    Q_OBJECT

public:
    explicit VideoSettings(QWidget *parent = nullptr);

    void setModel(EditorModel *m);

private:
    EditorModel   *model = nullptr;
    QComboBox     *deinterlaceCombo = nullptr;
    QRadioButton  *topFieldBtn = nullptr;
    QRadioButton  *bottomFieldBtn = nullptr;
};
