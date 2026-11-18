#pragma once

#include <QWidget>

class QPushButton;

class SourceColor : public QWidget
{
    Q_OBJECT

public:
    explicit SourceColor(QWidget *parent = nullptr);

    void setCurrentPreset(int presetIndex);

    void setTrashEnabled(bool on);

signals:
    void presetSelected(int presetIndex);
    void colorRequested();

private:
    QPushButton *colorButtons[8] = { nullptr };
    QPushButton *buttonSelect = nullptr;
    QPushButton *buttonTrash  = nullptr;

    int activePresetIndex = 0;

    void updatePresetButtonsVisual();
};
