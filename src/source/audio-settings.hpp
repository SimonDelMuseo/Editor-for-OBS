#pragma once
#include <QWidget>
#include "src/widgets/box.hpp"

#include <QVector>
#include <obs.h>

class QPushButton;
class QCheckBox;
class QSlider;
class QSpinBox;

class AudioSettings : public Box
{
    Q_OBJECT

public:
    explicit AudioSettings(QWidget *parent = nullptr);

    void loadFromSource(obs_source_t *src);
    void saveToSource(obs_source_t *src);

signals:
    void settingsChanged(const QString &undoKey);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    void refreshMixerState(obs_source_t *src);

private:
    QPushButton *hideInMixerButton = nullptr;

    QCheckBox *monoCheck = nullptr;
    QSlider   *balanceSlider = nullptr;
    QSpinBox  *syncOffsetSpin = nullptr;
    QCheckBox *monitorCheck = nullptr;

    QVector<QCheckBox*> trackChecks;
};
