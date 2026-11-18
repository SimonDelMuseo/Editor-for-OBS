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
#include <obs-frontend-api.h>

#include "editor-ui-helpers.hpp"

#include <cstring>

AudioSettings::AudioSettings(QWidget *parent)
    : Box(parent)
{
    auto *layout = qobject_cast<QVBoxLayout *>(this->layout());
    layout->setSpacing(6);

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(6);

        row->addStretch();

        hideInMixerButton = new QPushButton();
        hideInMixerButton->setObjectName("HideInMixer");
        hideInMixerButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        hideInMixerButton->setCheckable(true);

        hideInMixerButton->setAttribute(Qt::WA_TransparentForMouseEvents);
        hideInMixerButton->setFocusPolicy(Qt::NoFocus);
        hideInMixerButton->setCursor(Qt::ArrowCursor);

        disconnect(hideInMixerButton, nullptr, nullptr, nullptr);

        refreshMixerState(nullptr);

                
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

        balanceSlider->installEventFilter(this);

        row->addWidget(balanceSlider);


        auto *rightLabel = new QLabel(obs_module_text("Right"));
        rightLabel->setObjectName("Right");
        rightLabel->setStyleSheet("font-weight: bold;");
        row->addWidget(rightLabel);

        row->addStretch();

        layout->addLayout(row);
    }

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(6);

        row->addStretch();

        auto *monitorLabel = new QLabel(obs_module_text("AudioMonitoring"));
        monitorLabel->setObjectName("AudioMonitoring");
        monitorLabel->setStyleSheet("font-weight: bold;");
        row->addWidget(monitorLabel);

        row->addSpacing(4);

        monitorCheck = new QCheckBox();
        monitorCheck->setText(obs_module_text("Enable"));
        monitorCheck->setObjectName("AudioMonitoring");
        row->addWidget(monitorCheck);

        row->addStretch();

        layout->addLayout(row);
    }

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

    connect(hideInMixerButton, &QPushButton::clicked,
            this, [this]() { emit settingsChanged("UndoAudioHidden"); });

    connect(syncOffsetSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int) { emit settingsChanged("UndoSyncOffset"); });

    connect(monoCheck, &QCheckBox::stateChanged,
            this, [this](int) { emit settingsChanged("UndoMono"); });

    connect(balanceSlider, &QSlider::valueChanged,
            this, [this](int) { emit settingsChanged("UndoBalance"); });

    connect(monitorCheck, &QCheckBox::stateChanged,
            this, [this](int) { emit settingsChanged("UndoMonitoring"); });

    for (auto *cb : trackChecks) {
        connect(cb, &QCheckBox::stateChanged,
                this, [this](int) { emit settingsChanged("UndoTracks"); });
    }
}


static bool e33_is_source_in_preview(obs_source_t *src)
{
    if (!obs_frontend_preview_program_mode_active())
        return false;

    const char *uuid = obs_source_get_uuid(src);
    if (!uuid || !*uuid)
        return false;

    obs_source_t *previewSrc = obs_frontend_get_current_preview_scene();
    if (!previewSrc)
        return false;

    struct Search { const char *uuid; bool found; } search{uuid, false};

    auto enumItem = [](obs_scene_t *, obs_sceneitem_t *item,
                       void *param) -> bool {
        auto *s = static_cast<Search *>(param);
        if (!obs_sceneitem_visible(item))
            return true;
        obs_source_t *is = obs_sceneitem_get_source(item);
        if (!is)
            return true;
        if (!(obs_source_get_output_flags(is) & OBS_SOURCE_AUDIO))
            return true;
        if (!obs_source_audio_active(is))
            return true;
        const char *iu = obs_source_get_uuid(is);
        if (iu && *iu && strcmp(iu, s->uuid) == 0) {
            s->found = true;
            return false;
        }
        return true;
    };

    obs_scene_t *previewScene = obs_scene_from_source(previewSrc);
    if (previewScene)
        obs_scene_enum_items(previewScene, enumItem, &search);

    obs_source_release(previewSrc);
    return search.found;
}

void AudioSettings::refreshMixerState(obs_source_t *src)
{
    if (!hideInMixerButton)
        return;

    bool hidden = false, pinned = false, unassigned = false;
    bool active = false, previewed = false;

    if (src) {
        obs_data_t *priv = obs_source_get_private_settings(src);
        hidden = obs_data_get_bool(priv, "mixer_hidden");
        pinned = obs_data_get_bool(priv, "mixer_pinned");
        obs_data_release(priv);

        unassigned = (obs_source_get_audio_mixers(src) & 0x3F) == 0;
        active     = obs_source_active(src) && obs_source_audio_active(src);
        previewed  = e33_is_source_in_preview(src);
    }

    const char *state = "MixerState.Active";
    const char *cls   = "";

    if (unassigned) {
        state = "MixerState.Unassigned";
        cls   = "volume-unassigned";
    } else if (pinned) {
        state = "MixerState.Pinned";
        cls   = "volume-pinned";
    } else if (hidden) {
        state = "MixerState.Hidden";
        cls   = "volume-hidden";
    } else if (!active) {
        if (previewed) {
            state = "MixerState.Preview";
            cls   = "volume-preview";
        } else {
            state = "MixerState.Inactive";
            cls   = "volume-inactive";
        }
    }

    hideInMixerButton->setText(obs_module_text(state));
    hideInMixerButton->setChecked(hidden);

    QColor bg, fg;
    const editorui::MixerStateStyle th =
        editorui::GetThemeMixerStateStyle(cls);
    if (th.found) {
        bg = th.bg;
        fg = th.fg;
    } else {
        bg = QColor("#1e3a8a");
        fg = QColor("#ffffff");
        if (!strcmp(cls, "volume-hidden"))
            bg = QColor("#3d2380");
        else if (!strcmp(cls, "volume-inactive"))
            bg = QColor("#2b2b2b");
        else if (!strcmp(cls, "volume-preview"))
            bg = QColor("#14532d");
        else if (!strcmp(cls, "volume-unassigned"))
            bg = QColor("#7c2d12");
        else if (!strcmp(cls, "volume-pinned"))
            bg = QColor("#1e40af");
    }

    hideInMixerButton->setStyleSheet(
        QStringLiteral(
            "#HideInMixer {"
            "   border: 1px solid #5A5A5A;"
            "   padding: 3px 6px;"
            "   color: %1;"
            "   background-color: %2;"
            "}"
            "#HideInMixer:hover { background: transparent; }")
            .arg(fg.name(), bg.name()));
}

void AudioSettings::loadFromSource(obs_source_t *src)
{
    if (!src)
        return;

    refreshMixerState(src);

    {
        int64_t ns = obs_source_get_sync_offset(src);
        syncOffsetSpin->setValue(int(ns / 1000000));
    }

    {
        uint32_t flags = obs_source_get_flags(src);
        bool mono = (flags & OBS_SOURCE_FLAG_FORCE_MONO) != 0;
        monoCheck->setChecked(mono);
    }

    {
        float bal = obs_source_get_balance_value(src);
        int uiBal = int((bal - 0.5f) * 200.0f);
        balanceSlider->setValue(uiBal);
    }

    monitorCheck->setChecked(obs_source_get_monitoring_enabled(src));

    {
        uint32_t mask = obs_source_get_audio_mixers(src);
        for (int i = 0; i < trackChecks.size(); i++) {
            bool enabled = (mask & (1u << i)) != 0;
            trackChecks[i]->setChecked(enabled);
        }
    }
}

void AudioSettings::saveToSource(obs_source_t *src)
{
    if (!src)
        return;

    {
        bool hidden = hideInMixerButton->isChecked();

        obs_data_t *priv = obs_source_get_private_settings(src);
        obs_data_set_bool(priv, "mixer_hidden", hidden);
        obs_data_release(priv);
    }

    {
        int64_t ns = int64_t(syncOffsetSpin->value()) * 1000000;
        obs_source_set_sync_offset(src, ns);
    }

    {
        uint32_t flags = obs_source_get_flags(src);
        bool mono = monoCheck->isChecked();

        if (mono)
            flags |= OBS_SOURCE_FLAG_FORCE_MONO;
        else
            flags &= ~OBS_SOURCE_FLAG_FORCE_MONO;

        obs_source_set_flags(src, flags);
    }

    {
        float bal = (float(balanceSlider->value()) / 200.0f) + 0.5f;
        obs_source_set_balance_value(src, bal);
    }

    obs_source_set_monitoring_enabled(src, monitorCheck->isChecked());

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
        emit settingsChanged("UndoBalance");
        return true;
    }

    return Box::eventFilter(obj, event);
}
