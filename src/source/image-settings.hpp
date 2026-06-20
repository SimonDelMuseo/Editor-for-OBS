#pragma once
#include <QWidget>
#include "src/widgets/box.hpp"

class QComboBox;
class QPushButton;
class QMenu;
class EditorModel;   // ← añadido

class ImageSettings : public Box
{
    Q_OBJECT

public:
    explicit ImageSettings(QWidget *parent = nullptr);

    /* ────────────────────────────────────────────────
       MODEL — NECESARIO PARA EL BOTÓN DE SCREENSHOT
       ──────────────────────────────────────────────── */
    void setModel(EditorModel *m) { model = m; }

    /* ────────────────────────────────────────────────
       SCALE FILTERING — PUBLIC API
       ──────────────────────────────────────────────── */
    void setScaleFiltering(const QString &value);

    /* ────────────────────────────────────────────────
       WIDGETS PÚBLICOS PARA SOURCE-TAB
       ──────────────────────────────────────────────── */
    QComboBox *scaleFilteringCombo = nullptr;
    QComboBox *blendingModeCombo = nullptr;
    QComboBox *blendingMethodCombo = nullptr;

signals:
    void openProjectorOnMonitorRequested(int monitorIndex);
    void openProjectorWindowRequested();

    void scaleFilteringChanged(const QString &value);

private:
    QPushButton *saveScreenshotButton = nullptr;

    QPushButton *projectorMenuButton = nullptr;
    QMenu *projectorMenu = nullptr;

    EditorModel *model = nullptr;   // ← añadido
};
