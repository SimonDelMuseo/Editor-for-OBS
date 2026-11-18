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
#include <QScrollArea>
#include <QLabel>
#include <QWidget>
#include <QColorDialog>
#include <QCoreApplication>
#include <QScreen>
#include <QComboBox>
#include <QPropertyAnimation>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <obs-data.h>

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

SourceTab::SourceTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto *scrollWidget = new QWidget(scrollArea);
    auto *mainLayout = new QVBoxLayout(scrollWidget);

    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(4);

    scrollArea->setWidget(scrollWidget);
    outerLayout->addWidget(scrollArea);

    copyPaste = new CopyPaste(this);
    copyPaste->setModel(model);
    mainLayout->addWidget(copyPaste);

    sourceColor = new SourceColor(this);
    mainLayout->addWidget(sourceColor);

    connect(sourceColor, &SourceColor::presetSelected,
    this, [this, model](int preset) {
        if (!model)
            return;

        model->applyColorPresetToSelection(preset);
    });

    connect(sourceColor, &SourceColor::colorRequested,
        this, [this, model]() {
            if (!model)
                return;

            QColorDialog *dlg = new QColorDialog(this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->setOption(QColorDialog::ShowAlphaChannel);

            const QColor initial = model->currentItemColor();
            dlg->setCurrentColor(initial.isValid() ? initial : QColor("#55FF0000"));

            connect(dlg, &QColorDialog::colorSelected,
                this, [this](const QColor &c) {
                    applyCustomColor(c);
                });

            dlg->open();
        });

    transitions = new Transitions(model, this);
    mainLayout->addWidget(transitions);

    imageSettings = new ImageSettings(this);
    imageSettings->setModel(model);
    mainLayout->addWidget(imageSettings);

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

                obs_scale_type old = obs_sceneitem_get_scale_filter(item);
                if (old == filter)
                    return;

                obs_sceneitem_set_scale_filter(item, filter);
                model->pushItemPropUndo("UndoScaleFiltering", item,
                                        "scale_filter", old, filter, false);
            });

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

                obs_blending_type old = obs_sceneitem_get_blending_mode(item);
                if (old == mode)
                    return;

                obs_sceneitem_set_blending_mode(item, mode);
                model->pushItemPropUndo("UndoBlendingMode", item,
                                        "blend_mode", old, mode, false);
            });

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

                obs_blending_method old = obs_sceneitem_get_blending_method(item);
                if (old == method)
                    return;

                obs_sceneitem_set_blending_method(item, method);
                model->pushItemPropUndo("UndoBlendingMethod", item,
                                        "blend_method", old, method, false);
            });

    videoSettings = new VideoSettings(this);
    videoSettings->setModel(model);
    mainLayout->addWidget(videoSettings);

    audioSettings = new AudioSettings(this);
    mainLayout->addWidget(audioSettings);

    connect(audioSettings, &AudioSettings::settingsChanged,
            this, [this](const QString &undoKey) {
                if (!this->model)
                    return;

                obs_source_t *src = this->model->selectedSource();
                if (!src)
                    return;

                obs_data_t *before = this->model->captureAudioState(src);
                this->audioSettings->saveToSource(src);
                obs_data_t *after = this->model->captureAudioState(src);

                this->model->pushAudioUndo(undoKey.toUtf8().constData(),
                                           before, after, true);

                obs_data_release(before);
                obs_data_release(after);
            });


    mainLayout->addStretch();

    if (model) {
        connect(model, &EditorModel::modelUpdated,
                this, &SourceTab::refreshUI);
    }

    refreshUI();
}

void SourceTab::refreshUI()
{
    if (!model)
        return;

    obs_source_t *src = model->selectedSource();

    if (!src) {
        setContainerVisible(transitions,   false);
        setContainerVisible(imageSettings, false);
        setContainerVisible(videoSettings, false);
        setContainerVisible(audioSettings, false);

        setContainerVisible(sourceColor, false);
        setContainerVisible(copyPaste, copyPaste->hasClipboard());
        copyPaste->refreshStates();
        return;
    }

    setContainerVisible(sourceColor, true);
    setContainerVisible(copyPaste, true);
    copyPaste->refreshStates();

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

    setContainerVisible(imageSettings, showImage);
    setContainerVisible(videoSettings, showVideo);
    setContainerVisible(audioSettings, showAudio);
    setContainerVisible(transitions,   showTransitions);

    if (showAudio)
        audioSettings->loadFromSource(src);

    if (sourceColor) {
        int preset = model->currentItemPreset();
        sourceColor->setCurrentPreset(preset - 1);

        const bool hasColor =
            model->currentItemColor().isValid() || preset > 0;
        sourceColor->setTrashEnabled(hasColor);
    }

    copyPaste->refreshStates();
}

void SourceTab::setContainerVisible(QWidget *box, bool wantVisible)
{
    if (!box)
        return;

    QPropertyAnimation *prev = boxAnims.value(box, nullptr);
    const bool movingHide = prev && prev->endValue().toInt() == 0;

    if (wantVisible) {
        if (!box->isHidden() && !movingHide)
            return;

        if (prev) {
            boxAnims.remove(box);
            prev->stop();
            prev->deleteLater();
        }

        box->setMaximumHeight(0);
        box->show();
        startBoxAnim(box, 0, box->sizeHint().height(), false);
    } else {
        if (box->isHidden() || movingHide)
            return;

        if (prev) {
            boxAnims.remove(box);
            prev->stop();
            prev->deleteLater();
        }

        const int from = box->height() > 0 ? box->height()
                                           : box->sizeHint().height();
        startBoxAnim(box, from, 0, true);
    }
}

void SourceTab::startBoxAnim(QWidget *box, int from, int to, bool hideAtEnd)
{
    auto *anim = new QPropertyAnimation(box, "maximumHeight", box);
    anim->setDuration(250);
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(anim, &QPropertyAnimation::finished, this,
            [this, box, hideAtEnd]() {
                boxAnims.remove(box);
                if (hideAtEnd)
                    box->hide();
                box->setMaximumHeight(QWIDGETSIZE_MAX);
            });

    boxAnims.insert(box, anim);
    anim->start();
}

void SourceTab::applyCustomColor(const QColor &color)
{
    if (!model)
        return;

    model->applyCustomColorToSelection(color);
}
