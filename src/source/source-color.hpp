#pragma once

#include <QWidget>

class QPushButton;

class SourceColor : public QWidget
{
    Q_OBJECT

public:
    explicit SourceColor(QWidget *parent = nullptr);

    /* Sincronizar preset actual (0 = ninguno, 1..8 = presets OBS) */
    void setCurrentPreset(int presetIndex);

signals:
    void presetSelected(int presetIndex);  // ← NUEVO
    void colorRequested();                 // Custom color

private:
    QPushButton *colorButtons[8] = { nullptr };
    QPushButton *buttonSelect = nullptr;
    QPushButton *buttonTrash  = nullptr;

    int activePresetIndex = 0; // 0 = sin color, 1..8 = presets

    void updatePresetButtonsVisual();
};
