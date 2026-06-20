// video-settings.hpp
#pragma once
#include <QWidget>
#include "src/widgets/box.hpp"

/* ────────────────────────────────────────────────
   VIDEO SETTINGS — DESENTRELACADO
   ──────────────────────────────────────────────── */
class QComboBox;
class EditorModel;

class VideoSettings : public Box
{
    Q_OBJECT

public:
    explicit VideoSettings(QWidget *parent = nullptr);

    void setModel(EditorModel *m);

private:
    EditorModel *model = nullptr;
    QComboBox   *deinterlaceCombo = nullptr;
};
