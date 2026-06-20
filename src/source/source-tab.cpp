/* ────────────────────────────────────────────────
   INCLUDES
   ──────────────────────────────────────────────── */
#include "source-tab.hpp"
#include "editor-model.hpp"

#include "src/widgets/header.hpp"
#include "src/widgets/box.hpp"
#include "src/widgets/separator.hpp"
#include "transitions.hpp"
#include "copy-paste.hpp"
#include "source-color.hpp"
#include "audio-settings.hpp"
#include "image-settings.hpp"
#include "video-settings.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QColorDialog>
#include <QCoreApplication>
#include <QScreen>
#include <QComboBox>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <obs-data.h>

/* ────────────────────────────────────────────────
   COLORES REALES PARA PINTAR (MISMO ORDEN QUE OBS)
   ──────────────────────────────────────────────── */
static const char* presetColorsARGB[9] = {
    "",
    "#55FF0000",
    "#55FFFF00",
    "#5500FF00",
    "#5500FFFF",
    "#550000FF",
    "#55FF00FF",
    "#55666666",
    "#55000000"
};

/* ────────────────────────────────────────────────
   CONSTRUCTOR — SIN TÍTULOS Y SIN SEPARADOR
   ──────────────────────────────────────────────── */
SourceTab::SourceTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    auto *mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(4);

    /* COPY / PASTE */
    copyPaste = new CopyPaste(this);
    copyPaste->setModel(model);
    mainLayout->addWidget(copyPaste);

    /* SOURCE COLOR */
    sourceColor = new SourceColor(this);
    mainLayout->addWidget(sourceColor);

    /* ────────────────────────────────────────────────
       APLICAR PRESET
       ──────────────────────────────────────────────── */
    connect(sourceColor, &SourceColor::presetSelected,
    this, [this, model](int preset) {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *priv = obs_sceneitem_get_private_settings(item);
        if (!priv)
            return;

        if (preset == 0) {
            obs_data_set_int(priv, "color-preset", 0);
        } else {
            obs_data_set_int(priv, "color-preset", preset + 1);
        }

        obs_data_release(priv);

        model->setSelectedItem(item);

        obs_source_t *current = obs_frontend_get_current_scene();
        obs_source_t *fallback = nullptr;

        obs_frontend_source_list scenes = {};
        obs_frontend_get_scenes(&scenes);

        for (size_t i = 0; i < scenes.sources.num; i++) {
            if (scenes.sources.array[i] != current) {
                fallback = scenes.sources.array[i];
                break;
            }
        }

        if (fallback) {
            obs_frontend_set_current_scene(fallback);
            obs_frontend_set_current_scene(current);
        }

        obs_frontend_source_list_free(&scenes);
        obs_source_release(current);
    });

    /* ────────────────────────────────────────────────
       COLOR PERSONALIZADO (CORREGIDO)
       ──────────────────────────────────────────────── */
    connect(sourceColor, &SourceColor::colorRequested,
        this, [this, model]() {
            if (!model)
                return;

            obs_sceneitem_t *item = model->selectedItem();
            if (!item)
                return;

            obs_data_t *priv = obs_sceneitem_get_private_settings(item);
            if (!priv)
                return;

            const char *oldColor = obs_data_get_string(priv, "color");
            const char *initial = (oldColor && *oldColor != 0) ? oldColor : "#55FF0000";

            QColorDialog *dlg = new QColorDialog(QColor(initial), this);
            dlg->setOption(QColorDialog::ShowAlphaChannel);

            connect(dlg, &QColorDialog::colorSelected,
                this, [this](const QColor &c) {
                    applyCustomColor(c);
                });

            dlg->open();
            obs_data_release(priv);
        });

    /* TRANSITIONS */
    transitions = new Transitions(model, this);
    mainLayout->addWidget(transitions);

    /* IMAGE SETTINGS */
    imageSettings = new ImageSettings(this);
    imageSettings->setModel(model);
    mainLayout->addWidget(imageSettings);

    /* PROYECTOR */
    connect(imageSettings, &ImageSettings::openProjectorOnMonitorRequested,
            this, [this, model](int monitorIndex) {
                obs_source_t *src = model->selectedSource();
                if (!src)
                    return;

                const char *name = obs_source_get_name(src);

                obs_frontend_open_projector(
                    "Source",
                    monitorIndex,
                    nullptr,
                    name
                );
            });

    connect(imageSettings, &ImageSettings::openProjectorWindowRequested,
            this, [this, model]() {
                obs_source_t *src = model->selectedSource();
                if (!src)
                    return;

                const char *name = obs_source_get_name(src);

                obs_frontend_open_projector(
                    "Source",
                    -1,
                    nullptr,
                    name
                );
            });

    /* SCALE FILTERING */
    connect(imageSettings, &ImageSettings::scaleFilteringChanged,
            this, [this, model](const QString &value) {
                if (!model)
                    return;

                obs_sceneitem_t *item = model->selectedItem();
                if (!item)
                    return;

                obs_scale_type filter = OBS_SCALE_DISABLE;

                if (value == "disable")
                    filter = OBS_SCALE_DISABLE;
                else if (value == "point")
                    filter = OBS_SCALE_POINT;
                else if (value == "bilinear")
                    filter = OBS_SCALE_BILINEAR;
                else if (value == "bicubic")
                    filter = OBS_SCALE_BICUBIC;
                else if (value == "lanczos")
                    filter = OBS_SCALE_LANCZOS;
                else if (value == "area")
                    filter = OBS_SCALE_AREA;

                obs_sceneitem_set_scale_filter(item, filter);
            });

    /* BLENDING MODE */
    connect(imageSettings->blendingModeCombo, &QComboBox::currentIndexChanged,
            this, [this, model](int index) {
                if (!model)
                    return;

                obs_sceneitem_t *item = model->selectedItem();
                if (!item)
                    return;

                obs_blending_type mode = OBS_BLEND_NORMAL;

                switch (index) {
                case 0: mode = OBS_BLEND_NORMAL;     break;
                case 1: mode = OBS_BLEND_ADDITIVE;   break;
                case 2: mode = OBS_BLEND_SUBTRACT;   break;
                case 3: mode = OBS_BLEND_SCREEN;     break;
                case 4: mode = OBS_BLEND_MULTIPLY;   break;
                case 5: mode = OBS_BLEND_LIGHTEN;    break;
                case 6: mode = OBS_BLEND_DARKEN;     break;
                }

                obs_sceneitem_set_blending_mode(item, mode);
            });

    /* BLENDING METHOD */
    connect(imageSettings->blendingMethodCombo, &QComboBox::currentIndexChanged,
            this, [this, model](int index) {
                if (!model)
                    return;

                obs_sceneitem_t *item = model->selectedItem();
                if (!item)
                    return;

                obs_blending_method method = OBS_BLEND_METHOD_DEFAULT;

                switch (index) {
                case 0: method = OBS_BLEND_METHOD_DEFAULT;   break;
                case 1: method = OBS_BLEND_METHOD_SRGB_OFF;  break;
                }

                obs_sceneitem_set_blending_method(item, method);
            });

    /* VIDEO SETTINGS */
    videoSettings = new VideoSettings(this);
    videoSettings->setModel(model);
    mainLayout->addWidget(videoSettings);

    /* AUDIO SETTINGS */
    audioSettings = new AudioSettings(this);
    mainLayout->addWidget(audioSettings);

    /* GUARDAR CAMBIOS DE AUDIO */
    connect(audioSettings, &AudioSettings::settingsChanged,
            this, [this]() {
                if (!this->model)
                    return;

                obs_source_t *src = this->model->selectedSource();
                if (!src)
                    return;

                this->audioSettings->saveToSource(src);
            });

    /* ──────────────────────────────────────────────── */

    mainLayout->addStretch();

    if (model) {
        connect(model, &EditorModel::modelUpdated,
                this, &SourceTab::refreshUI);
    }

    refreshUI();
}

/* ────────────────────────────────────────────────
   REFRESCAR UI
   ──────────────────────────────────────────────── */
void SourceTab::refreshUI()
{
    if (!model)
        return;

    obs_source_t *src = model->selectedSource();

    if (!src) {
        transitions->setVisible(false);
        imageSettings->setVisible(false);
        videoSettings->setVisible(false);
        audioSettings->setVisible(false);
        return;
    }

    /* SCALE FILTERING — READ FROM OBS */
    {
        obs_sceneitem_t *item = model->selectedItem();
        if (item) {
            obs_scale_type filter = obs_sceneitem_get_scale_filter(item);
            QString key = "disable";

            switch (filter) {
            case OBS_SCALE_DISABLE:   key = "disable";  break;
            case OBS_SCALE_POINT:     key = "point";    break;
            case OBS_SCALE_BILINEAR:  key = "bilinear"; break;
            case OBS_SCALE_BICUBIC:   key = "bicubic";  break;
            case OBS_SCALE_LANCZOS:   key = "lanczos";  break;
            case OBS_SCALE_AREA:      key = "area";     break;
            default:                  key = "disable";  break;
            }

            imageSettings->setScaleFiltering(key);
        }
    }

    /* BLENDING MODE — READ FROM OBS */
    {
        obs_sceneitem_t *item = model->selectedItem();
        if (item) {
            obs_blending_type mode = obs_sceneitem_get_blending_mode(item);
            int index = 0;

            switch (mode) {
            case OBS_BLEND_NORMAL:     index = 0; break;
            case OBS_BLEND_ADDITIVE:   index = 1; break;
            case OBS_BLEND_SUBTRACT:   index = 2; break;
            case OBS_BLEND_SCREEN:     index = 3; break;
            case OBS_BLEND_MULTIPLY:   index = 4; break;
            case OBS_BLEND_LIGHTEN:    index = 5; break;
            case OBS_BLEND_DARKEN:     index = 6; break;
            default:                   index = 0; break;
            }

            QSignalBlocker blocker(imageSettings->blendingModeCombo);
            imageSettings->blendingModeCombo->setCurrentIndex(index);
        }
    }

    /* BLENDING METHOD — READ FROM OBS */
    {
        obs_sceneitem_t *item = model->selectedItem();
        if (item) {
            obs_blending_method method = obs_sceneitem_get_blending_method(item);
            int index = 0;

            switch (method) {
            case OBS_BLEND_METHOD_DEFAULT:   index = 0; break;
            case OBS_BLEND_METHOD_SRGB_OFF:  index = 1; break;
            default:                         index = 0; break;
            }

            QSignalBlocker blocker(imageSettings->blendingMethodCombo);
            imageSettings->blendingMethodCombo->setCurrentIndex(index);
        }
    }

    QString type = model->sourceType();
    uint32_t flags = obs_source_get_output_flags(src);

    bool hasVideo = flags & OBS_SOURCE_VIDEO;
    bool hasAudio = flags & OBS_SOURCE_AUDIO;
    bool isAsync  = flags & OBS_SOURCE_ASYNC;

    bool isText  = (type == "text_gdiplus" || type == "text_ft2_source");
    bool isScene = (type == "scene");
    bool isGroup = (type == "group");

    bool isVisual =
        hasVideo ||
        isAsync ||
        isText ||
        isScene ||
        isGroup ||
        type == "browser_source";

    bool showImage = isVisual;
    bool showAudio = hasAudio;

    bool showVideo =
        (type == "ffmpeg_source" ||
         type == "vlc_source"    ||
         type == "dshow_input");

    bool showTransitions = isVisual;

    imageSettings->setVisible(showImage);
    videoSettings->setVisible(showVideo);
    audioSettings->setVisible(showAudio);
    transitions->setVisible(showTransitions);

    /* CARGAR AUDIO DESDE OBS */
    if (showAudio)
        audioSettings->loadFromSource(src);

    if (sourceColor) {
        int preset = model->currentItemPreset();
        sourceColor->setCurrentPreset(preset - 1);
    }
}

/* ────────────────────────────────────────────────
   APLICAR COLOR PERSONALIZADO
   ──────────────────────────────────────────────── */
void SourceTab::applyCustomColor(const QColor &color)
{
    if (!model)
        return;

    obs_sceneitem_t *item = model->selectedItem();
    if (!item)
        return;

    if (!color.isValid())
        return;

    obs_data_t *priv = obs_sceneitem_get_private_settings(item);
    if (!priv)
        return;

    QString argb = color.name(QColor::HexArgb);
    obs_data_set_string(priv, "color", argb.toUtf8().constData());
    obs_data_set_int(priv, "color-preset", 1);

    obs_data_release(priv);
    model->setSelectedItem(item);

    obs_source_t *current = obs_frontend_get_current_scene();
    obs_source_t *fallback = nullptr;

    obs_frontend_source_list scenes = {};
    obs_frontend_get_scenes(&scenes);

    for (size_t i = 0; i < scenes.sources.num; i++) {
        if (scenes.sources.array[i] != current) {
            fallback = scenes.sources.array[i];
            break;
        }
    }

    if (fallback) {
        obs_frontend_set_current_scene(fallback);
        obs_frontend_set_current_scene(current);
    }

    obs_frontend_source_list_free(&scenes);
    obs_source_release(current);
}
