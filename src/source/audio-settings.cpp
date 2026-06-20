#include "src/source/audio-settings.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QEvent>

#include <obs-module.h>
#include <obs.h>
#include <obs-source.h>

/* ────────────────────────────────────────────────
   AUDIO SETTINGS — UI ELEMENTS (4 LÍNEAS)
   ──────────────────────────────────────────────── */
AudioSettings::AudioSettings(QWidget *parent)
    : Box(parent)
{
    auto *layout = qobject_cast<QVBoxLayout *>(this->layout());
    layout->setSpacing(6);

    /* ────────────────────────────────────────────────
       LÍNEA 1:
       Hide in Mixer + Sync Offset
       ──────────────────────────────────────────────── */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(6);

        row->addStretch();

        hideInMixerButton = new QPushButton();
        hideInMixerButton->setObjectName("HideInMixer");
        hideInMixerButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        hideInMixerButton->setCheckable(true);

        /* Desactivar interacción completamente */
        hideInMixerButton->setAttribute(Qt::WA_TransparentForMouseEvents);
        hideInMixerButton->setFocusPolicy(Qt::NoFocus);
        hideInMixerButton->setCursor(Qt::ArrowCursor);

        /* Quitar hover y cualquier efecto visual */
        hideInMixerButton->setStyleSheet(
        "#HideInMixer {"
        "   border: 1px solid #5A5A5A;"      /* borde gris medio */
        "   padding: 3px 6px;"
        "   color: white;"
        "   background-color: #1e3a8a;"      /* Active por defecto */
        "}"
        "#HideInMixer:checked {"
        "   background-color: #3d2380;"      /* Hidden */
        "}"
        "#HideInMixer:hover { background: transparent; }"
    );


        /* No debe emitir nada al hacer click */
        disconnect(hideInMixerButton, nullptr, nullptr, nullptr);

        /* Texto inicial (se actualizará en loadFromSource) */
        hideInMixerButton->setText(obs_module_text("VisibleInMixer"));

                
        row->addWidget(hideInMixerButton);

        row->addSpacing(12);

        auto *syncLabel = new QLabel(obs_module_text("SyncOffset"));
        syncLabel->setObjectName("SyncOffset");
        syncLabel->setStyleSheet("font-weight: bold;");
        row->addWidget(syncLabel);

        row->addSpacing(4);

        syncOffsetSpin = new QSpinBox();
        syncOffsetSpin->setRange(-10000, 10000);
        syncOffsetSpin->setValue(0);
        syncOffsetSpin->setSuffix(" ms");
        syncOffsetSpin->setFixedWidth(90);
        row->addWidget(syncOffsetSpin);

        row->addStretch();

        layout->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       LÍNEA 2:
       Mono + Balance (L / slider / R)
       ──────────────────────────────────────────────── */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(6);

        row->addStretch();

        auto *monoLabel = new QLabel(obs_module_text("Mono"));
        monoLabel->setObjectName("Mono");
        monoLabel->setStyleSheet("font-weight: bold;");
        row->addWidget(monoLabel);

        row->addSpacing(4);

        monoCheck = new QCheckBox();
        row->addWidget(monoCheck);

        row->addSpacing(12);

        auto *balanceLabel = new QLabel(obs_module_text("Balance"));
        balanceLabel->setObjectName("Balance");
        balanceLabel->setStyleSheet("font-weight: bold;");
        row->addWidget(balanceLabel);

        row->addSpacing(4);

        auto *leftLabel = new QLabel(obs_module_text("Left"));
        leftLabel->setObjectName("Left");
        leftLabel->setStyleSheet("font-weight: bold;");
        row->addWidget(leftLabel);

        balanceSlider = new QSlider(Qt::Horizontal);
        balanceSlider->setRange(-100, 100);
        balanceSlider->setValue(0);
        balanceSlider->setFixedWidth(120);
        balanceSlider->setSingleStep(1);
        balanceSlider->setPageStep(10);

        /* PASO 1 — habilitar doble clic */
        balanceSlider->installEventFilter(this);

        row->addWidget(balanceSlider);


        auto *rightLabel = new QLabel(obs_module_text("Right"));
        rightLabel->setObjectName("Right");
        rightLabel->setStyleSheet("font-weight: bold;");
        row->addWidget(rightLabel);

        row->addStretch();

        layout->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       LÍNEA 3:
       Audio Monitoring + ComboBox
       ──────────────────────────────────────────────── */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(6);

        row->addStretch();

        auto *monitorLabel = new QLabel(obs_module_text("AudioMonitoring"));
        monitorLabel->setObjectName("AudioMonitoring");
        monitorLabel->setStyleSheet("font-weight: bold;");
        row->addWidget(monitorLabel);

        row->addSpacing(4);

        monitoringCombo = new QComboBox();
        monitoringCombo->addItem(obs_module_text("MonitorOff"));
        monitoringCombo->addItem(obs_module_text("MonitorOnlyMute"));
        monitoringCombo->addItem(obs_module_text("MonitorAndOutput"));
        monitoringCombo->setObjectName("AudioMonitoring");
        row->addWidget(monitoringCombo);

        row->addStretch();

        layout->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       LÍNEA 4:
       Tracks (6 checkboxes)
       ──────────────────────────────────────────────── */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(6);

        row->addStretch();

        auto *tracksLabel = new QLabel(obs_module_text("Tracks"));
        tracksLabel->setObjectName("Tracks");
        tracksLabel->setStyleSheet("font-weight: bold;");
        row->addWidget(tracksLabel);

        row->addSpacing(4);

        for (int i = 0; i < 6; ++i) {
            QCheckBox *cb = new QCheckBox(QString::number(i + 1));
            cb->setChecked(true);
            cb->setObjectName(QString("Track%1").arg(i + 1));
            trackChecks.append(cb);

            row->addWidget(cb);
            row->addSpacing(3);
        }

        row->addStretch();

        layout->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       CONEXIONES
       ──────────────────────────────────────────────── */
    connect(hideInMixerButton, &QPushButton::clicked,
            this, [this]() { emit settingsChanged(); });

    connect(syncOffsetSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int) { emit settingsChanged(); });

    connect(monoCheck, &QCheckBox::stateChanged,
            this, [this](int) { emit settingsChanged(); });

    connect(balanceSlider, &QSlider::valueChanged,
            this, [this](int) { emit settingsChanged(); });

    connect(monitoringCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) { emit settingsChanged(); });

    for (auto *cb : trackChecks) {
        connect(cb, &QCheckBox::stateChanged,
                this, [this](int) { emit settingsChanged(); });
    }
}

/* ────────────────────────────────────────────────
   CARGAR DESDE OBS
   ──────────────────────────────────────────────── */
void AudioSettings::loadFromSource(obs_source_t *src)
{
    if (!src)
        return;

    /* HIDE IN MIXER */
    {
        obs_data_t *priv = obs_source_get_private_settings(src);
        bool hidden = obs_data_get_bool(priv, "mixer_hidden");
        obs_data_release(priv);

        hideInMixerButton->setChecked(hidden);

        if (hidden)
            hideInMixerButton->setText(obs_module_text("HiddenInMixer"));
        else
            hideInMixerButton->setText(obs_module_text("ActiveInMixer"));
    }

    /* SYNC OFFSET (ns → ms) */
    {
        int64_t ns = obs_source_get_sync_offset(src);
        syncOffsetSpin->setValue(int(ns / 1000000));
    }

    /* MONO */
    {
        uint32_t flags = obs_source_get_flags(src);
        bool mono = (flags & OBS_SOURCE_FLAG_FORCE_MONO) != 0;
        monoCheck->setChecked(mono);
    }

    /* BALANCE (-1.0 → 1.0) */
    {
        float bal = obs_source_get_balance_value(src);
        /* 0..1  →  -100..100 */
        int uiBal = int((bal - 0.5f) * 200.0f);
        balanceSlider->setValue(uiBal);
    }

    /* MONITORING */
    {
        int type = (int)obs_source_get_monitoring_type(src);
        if (type < 0)
            type = 0;
        if (type > 2)
            type = 0;
        monitoringCombo->setCurrentIndex(type);
    }

    /* TRACKS */
    {
        uint32_t mask = obs_source_get_audio_mixers(src);
        for (int i = 0; i < trackChecks.size(); i++) {
            bool enabled = (mask & (1u << i)) != 0;
            trackChecks[i]->setChecked(enabled);
        }
    }
}

/* ────────────────────────────────────────────────
   GUARDAR EN OBS
   ──────────────────────────────────────────────── */
void AudioSettings::saveToSource(obs_source_t *src)
{
    if (!src)
        return;

    /* HIDE IN MIXER */
    {
        bool hidden = hideInMixerButton->isChecked();

        obs_data_t *priv = obs_source_get_private_settings(src);
        obs_data_set_bool(priv, "mixer_hidden", hidden);
        obs_data_release(priv);
    }

    /* SYNC OFFSET (ms → ns) */
    {
        int64_t ns = int64_t(syncOffsetSpin->value()) * 1000000;
        obs_source_set_sync_offset(src, ns);
    }

    /* MONO */
    {
        uint32_t flags = obs_source_get_flags(src);
        bool mono = monoCheck->isChecked();

        if (mono)
            flags |= OBS_SOURCE_FLAG_FORCE_MONO;
        else
            flags &= ~OBS_SOURCE_FLAG_FORCE_MONO;

        obs_source_set_flags(src, flags);
    }

    /* BALANCE */
    {
        /* -100..100  →  0..1 */
        float bal = (float(balanceSlider->value()) / 200.0f) + 0.5f;
        obs_source_set_balance_value(src, bal);
    }

    /* MONITORING */
    {
        int type = monitoringCombo->currentIndex();
        obs_source_set_monitoring_type(src, (obs_monitoring_type)type);
    }

    /* TRACKS */
    {
        uint32_t mask = 0;
        for (int i = 0; i < trackChecks.size(); i++) {
            if (trackChecks[i]->isChecked())
                mask |= (1u << i);
        }
        obs_source_set_audio_mixers(src, mask);
    }
}

bool AudioSettings::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == balanceSlider && event->type() == QEvent::MouseButtonDblClick) {
        balanceSlider->setValue(0);
        emit settingsChanged();
        return true;
    }

    return Box::eventFilter(obj, event);
}
