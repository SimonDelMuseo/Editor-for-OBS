#pragma once
#include <QWidget>
#include "src/widgets/box.hpp"

#include <QVector>
#include <obs.h>

/* Forward Qt */
class QPushButton;
class QCheckBox;
class QSlider;
class QSpinBox;
class QComboBox;

/* ────────────────────────────────────────────────
   AUDIO SETTINGS — DECLARACIÓN DE ELEMENTOS UI
   ──────────────────────────────────────────────── */
class AudioSettings : public Box
{
    Q_OBJECT

public:
    explicit AudioSettings(QWidget *parent = nullptr);

    /* CARGAR Y GUARDAR DESDE/HACIA OBS */
    void loadFromSource(obs_source_t *src);
    void saveToSource(obs_source_t *src);

signals:
    void settingsChanged();   // SourceTab lo usará para guardar

protected:
    /* PASO 1 — necesario para detectar doble clic */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QPushButton *hideInMixerButton = nullptr;

    QCheckBox *monoCheck = nullptr;
    QSlider   *balanceSlider = nullptr;
    QSpinBox  *syncOffsetSpin = nullptr;
    QComboBox *monitoringCombo = nullptr;

    QVector<QCheckBox*> trackChecks;
};
