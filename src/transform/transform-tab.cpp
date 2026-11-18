#include "transform-tab.hpp"
#include "editor-model.hpp"
#include "editor-ui-helpers.hpp"

#include "copypaste.hpp"
#include "quick.hpp"
#include "alignment.hpp"
#include "edit.hpp"

#include "edit-position.hpp"
#include "edit-bounds.hpp"
#include "edit-crop.hpp"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QAction>
#include <QPropertyAnimation>

#include <cmath>
#include <graphics/matrix4.h>

static const uint32_t indexToAlign[] = {
    OBS_ALIGN_TOP | OBS_ALIGN_LEFT,
    OBS_ALIGN_TOP,
    OBS_ALIGN_TOP | OBS_ALIGN_RIGHT,
    OBS_ALIGN_LEFT,
    OBS_ALIGN_CENTER,
    OBS_ALIGN_RIGHT,
    OBS_ALIGN_BOTTOM | OBS_ALIGN_LEFT,
    OBS_ALIGN_BOTTOM,
    OBS_ALIGN_BOTTOM | OBS_ALIGN_RIGHT
};

static int alignToIndex(uint32_t align)
{
    for (int i = 0; i < 9; i++)
        if (indexToAlign[i] == align)
            return i;
    return 0;
}

static vec2 getAlignmentConversion(uint32_t alignment)
{
    vec2 ratio = {0.5f, 0.5f};
    if (alignment & OBS_ALIGN_RIGHT)
        ratio.x = 1.0f;
    if (alignment & OBS_ALIGN_LEFT)
        ratio.x = 0.0f;
    if (alignment & OBS_ALIGN_BOTTOM)
        ratio.y = 1.0f;
    if (alignment & OBS_ALIGN_TOP)
        ratio.y = 0.0f;
    return ratio;
}

TransformTab::TransformTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto *scrollWidget = new QWidget(scrollArea);
    auto *layout = new QVBoxLayout(scrollWidget);

    copyPasteBox = new CopyPasteBox(this);
    quickBox     = new QuickBox(this);
    alignmentBox = new AlignmentBox(this);
    editBox      = new EditBox(this);

    layout->addWidget(copyPasteBox);
    layout->addWidget(quickBox);
    layout->addWidget(alignmentBox);
    layout->addWidget(editBox);

    layout->addStretch();

    scrollArea->setWidget(scrollWidget);
    outerLayout->addWidget(scrollArea);

        connect(copyPasteBox, &CopyPasteBox::copyTransform,
                this, [this](){ this->model->copyTransform(); });

        connect(copyPasteBox, &CopyPasteBox::pasteTransform,
                this, [this](){ this->model->pasteTransform(); });

        connect(copyPasteBox, &CopyPasteBox::resetTransform,
                this, [this](){ this->model->resetTransform(); });

    connect(quickBox, &QuickBox::flipHorizontal,
            model, &EditorModel::flipHorizontal);

    connect(quickBox, &QuickBox::flipVertical,
            model, &EditorModel::flipVertical);

    connect(quickBox, &QuickBox::rotate90CW,
            model, &EditorModel::rotate90CW);

    connect(quickBox, &QuickBox::rotate90CCW,
            model, &EditorModel::rotate90CCW);

    connect(quickBox, &QuickBox::rotate180,
            model, &EditorModel::rotate180);

    connect(alignmentBox, &AlignmentBox::fitToScreen,
            model, &EditorModel::fitToScreen);

    connect(alignmentBox, &AlignmentBox::stretchToScreen,
            model, &EditorModel::stretchToScreen);

    connect(alignmentBox, &AlignmentBox::centerToScreen,
            model, &EditorModel::centerToScreen);

    connect(alignmentBox, &AlignmentBox::centerVertical,
            model, &EditorModel::centerVertical);

    connect(alignmentBox, &AlignmentBox::centerHorizontal,
            model, &EditorModel::centerHorizontal);


    auto *pos    = editBox->findChild<EditPosition*>();
    auto *bounds = editBox->findChild<EditBounds*>();
    auto *crop   = editBox->findChild<EditCrop*>();

    connect(pos, &EditPosition::posXChanged,
            model, [model](double v){
        model->editTransform("UndoTransformEdit", [v](obs_sceneitem_t *item){
            obs_transform_info oti;
            obs_sceneitem_get_info2(item, &oti);
            oti.pos.x = float(v);
            obs_sceneitem_set_info2(item, &oti);
        });
    });

    connect(pos, &EditPosition::posYChanged,
            model, [model](double v){
        model->editTransform("UndoTransformEdit", [v](obs_sceneitem_t *item){
            obs_transform_info oti;
            obs_sceneitem_get_info2(item, &oti);
            oti.pos.y = float(v);
            obs_sceneitem_set_info2(item, &oti);
        });
    });

    connect(pos, &EditPosition::widthChanged,
            model, [model](double v){
        obs_source_t *src = model->selectedSource();
        if (!src) return;
        const uint32_t w = obs_source_get_width(src);
        if (w == 0) return;

        model->editTransform("UndoTransformEdit", [v, w](obs_sceneitem_t *item){
            vec2 scale;
            obs_sceneitem_get_scale(item, &scale);
            scale.x = float(v) / float(w);
            obs_sceneitem_set_scale(item, &scale);
        });
    });

    connect(pos, &EditPosition::heightChanged,
            model, [model](double v){
        obs_source_t *src = model->selectedSource();
        if (!src) return;
        const uint32_t h = obs_source_get_height(src);
        if (h == 0) return;

        model->editTransform("UndoTransformEdit", [v, h](obs_sceneitem_t *item){
            vec2 scale;
            obs_sceneitem_get_scale(item, &scale);
            scale.y = float(v) / float(h);
            obs_sceneitem_set_scale(item, &scale);
        });
    });

    connect(pos, &EditPosition::rotationChanged,
            model, [model](double v){
        model->editTransform("UndoTransformEdit", [v](obs_sceneitem_t *item){
            obs_transform_info oti;
            obs_sceneitem_get_info2(item, &oti);
            oti.rot = float(v);
            obs_sceneitem_set_info2(item, &oti);
        });
    });

    connect(pos, &EditPosition::alignmentChanged,
            model, [model](int index){
        model->editTransform("UndoTransformEdit", [index](obs_sceneitem_t *item){
            if (index < 0 || index > 8)
                return;

            const uint32_t alignment = indexToAlign[index];
            const vec2 flipRatio = getAlignmentConversion(alignment);

            obs_transform_info oti;
            obs_sceneitem_get_info2(item, &oti);

            obs_sceneitem_crop crop;
            obs_sceneitem_get_crop(item, &crop);

            obs_source_t *source = obs_sceneitem_get_source(item);
            const uint32_t sourceWidth  = obs_source_get_width(source);
            const uint32_t sourceHeight = obs_source_get_height(source);

            uint32_t widthForFlip  = sourceWidth  - crop.left - crop.right;
            uint32_t heightForFlip = sourceHeight - crop.top  - crop.bottom;
            if (oti.bounds_type != OBS_BOUNDS_NONE) {
                widthForFlip  = (uint32_t)oti.bounds.x;
                heightForFlip = (uint32_t)oti.bounds.y;
            }

            const vec2 currentRatio = getAlignmentConversion(oti.alignment);
            const float shiftX =
                (currentRatio.x - flipRatio.x) * (float)widthForFlip  * oti.scale.x;
            const float shiftY =
                (currentRatio.y - flipRatio.y) * (float)heightForFlip * oti.scale.y;

            oti.pos.x -= shiftX;
            oti.pos.y -= shiftY;
            oti.alignment = alignment;
            obs_sceneitem_set_info2(item, &oti);
        });
    });

    connect(bounds, &EditBounds::boundsTypeChanged,
            model, [model](int index){
        if (index < 0)
            return;
        obs_source_t *source = model->selectedSource();
        if (!source)
            return;

        model->editTransform("UndoTransformEdit", [index](obs_sceneitem_t *item){
            obs_transform_info oti;
            obs_sceneitem_get_info2(item, &oti);

            const obs_bounds_type lastType = oti.bounds_type;
            const obs_bounds_type newType  = (obs_bounds_type)index;

            obs_source_t *src = obs_sceneitem_get_source(item);
            const int width  = (int)obs_source_get_width(src);
            const int height = (int)obs_source_get_height(src);

            obs_sceneitem_crop crop;
            obs_sceneitem_get_crop(item, &crop);

            if (lastType == OBS_BOUNDS_NONE && newType != OBS_BOUNDS_NONE) {
                vec2 scale;
                obs_sceneitem_get_scale(item, &scale);

                oti.scale.x = 1.0f;
                oti.scale.y = 1.0f;
                oti.bounds.x =
                    float(width  - (int)crop.left - (int)crop.right)  * scale.x;
                oti.bounds.y =
                    float(height - (int)crop.top  - (int)crop.bottom) * scale.y;
            } else if (lastType != OBS_BOUNDS_NONE && newType == OBS_BOUNDS_NONE) {
                matrix4 draw;
                obs_sceneitem_get_draw_transform(item, &draw);

                const vec2 currentRatio =
                    getAlignmentConversion(OBS_ALIGN_TOP | OBS_ALIGN_LEFT);
                const vec2 flipRatio = getAlignmentConversion(oti.alignment);

                const uint32_t widthForFlip  = width  - crop.left - crop.right;
                const uint32_t heightForFlip = height - crop.top  - crop.bottom;

                const float shiftX =
                    (currentRatio.x - flipRatio.x) * (float)widthForFlip * draw.x.x;
                const float shiftY =
                    (currentRatio.y - flipRatio.y) * (float)heightForFlip * draw.y.y;

                oti.scale.x = draw.x.x;
                oti.scale.y = draw.y.y;
                oti.pos.x   = draw.t.x - shiftX;
                oti.pos.y   = draw.t.y - shiftY;
            }

            oti.bounds_type = newType;
            obs_sceneitem_set_info2(item, &oti);
        });
    });

        connect(bounds, &EditBounds::boundsWidthChanged,
                model, [model](double v){
            model->editTransform("UndoTransformEdit", [v](obs_sceneitem_t *item){
                obs_transform_info oti;
                obs_sceneitem_get_info2(item, &oti);
                oti.bounds.x = float(v);
                obs_sceneitem_set_info2(item, &oti);
            });
        });

        connect(bounds, &EditBounds::boundsHeightChanged,
                model, [model](double v){
            model->editTransform("UndoTransformEdit", [v](obs_sceneitem_t *item){
                obs_transform_info oti;
                obs_sceneitem_get_info2(item, &oti);
                oti.bounds.y = float(v);
                obs_sceneitem_set_info2(item, &oti);
            });
        });

        connect(bounds, &EditBounds::cropToBoundsChanged,
                model, [model](bool enabled){
            model->editTransform("UndoTransformEdit", [enabled](obs_sceneitem_t *item){
                obs_transform_info oti;
                obs_sceneitem_get_info2(item, &oti);
                oti.crop_to_bounds = enabled;
                obs_sceneitem_set_info2(item, &oti);
            });
        });

        connect(bounds, &EditBounds::alignmentChanged,
                model, [model](int index){
            model->editTransform("UndoTransformEdit", [index](obs_sceneitem_t *item){
                obs_transform_info oti;
                obs_sceneitem_get_info2(item, &oti);
                oti.bounds_alignment = indexToAlign[index];
                obs_sceneitem_set_info2(item, &oti);
            });
        });


    connect(crop, &EditCrop::cropLeftChanged,
            model, [model](double v){
        model->editTransform("UndoTransformEdit", [v](obs_sceneitem_t *item){
            obs_sceneitem_crop c;
            obs_sceneitem_get_crop(item, &c);
            c.left = int(v);
            obs_sceneitem_set_crop(item, &c);
        });
    });

    connect(crop, &EditCrop::cropTopChanged,
            model, [model](double v){
        model->editTransform("UndoTransformEdit", [v](obs_sceneitem_t *item){
            obs_sceneitem_crop c;
            obs_sceneitem_get_crop(item, &c);
            c.top = int(v);
            obs_sceneitem_set_crop(item, &c);
        });
    });

    connect(crop, &EditCrop::cropRightChanged,
            model, [model](double v){
        model->editTransform("UndoTransformEdit", [v](obs_sceneitem_t *item){
            obs_sceneitem_crop c;
            obs_sceneitem_get_crop(item, &c);
            c.right = int(v);
            obs_sceneitem_set_crop(item, &c);
        });
    });

    connect(crop, &EditCrop::cropBottomChanged,
            model, [model](double v){
        model->editTransform("UndoTransformEdit", [v](obs_sceneitem_t *item){
            obs_sceneitem_crop c;
            obs_sceneitem_get_crop(item, &c);
            c.bottom = int(v);
            obs_sceneitem_set_crop(item, &c);
        });
    });

    connect(model, &EditorModel::modelUpdated,
            this, &TransformTab::refreshUI);

    connect(model, &EditorModel::transformChanged,
            this, &TransformTab::refreshUI);
}

void TransformTab::refreshUI()
{
    const bool hasSource = model->selectedItem() != nullptr;

    setContainerVisible(copyPasteBox, hasSource);
    setContainerVisible(quickBox,     hasSource);
    setContainerVisible(alignmentBox, hasSource);
    setContainerVisible(editBox,      hasSource);

    if (!hasSource)
        return;

    {
        obs_sceneitem_t *item = model->selectedItem();
        obs_source_t *src = obs_sceneitem_get_source(item);
        const char *id = obs_source_get_unversioned_id(src);

        bool isAudio =
            strcmp(id, "wasapi_input_capture")  == 0 ||
            strcmp(id, "wasapi_output_capture") == 0 ||
            strcmp(id, "pulse_input_capture")   == 0 ||
            strcmp(id, "pulse_output_capture")  == 0 ||
            strcmp(id, "coreaudio_input_capture")  == 0 ||
            strcmp(id, "coreaudio_output_capture") == 0 ||
            strcmp(id, "audio_input_capture")   == 0 ||
            strcmp(id, "audio_output_capture")  == 0;

        if (isAudio) {
            copyPasteBox->setEnabled(false);
            quickBox->setEnabled(false);
            alignmentBox->setEnabled(false);
            editBox->setEnabled(false);
            return;
        }
    }
   

    const bool locked = model->isLocked();

    QAction *nativePaste =
        editorui::FindNativeAction("actionPasteTransform");
    const bool ownBuffer =
        model->clipboardTransition &&
        !obs_data_has_user_value(model->clipboardTransition,
                                 "transition_id");
    const bool hasCopied =
        (nativePaste && nativePaste->isEnabled()) || ownBuffer;

    copyPasteBox->setEnabled(true);
    copyPasteBox->setStatesEnabled(true, !locked && hasCopied, !locked);

    quickBox->setEnabled(!locked);
    alignmentBox->setEnabled(!locked);
    editBox->setEnabled(!locked);

    obs_sceneitem_t *item = model->selectedItem();
    obs_transform_info oti;
    obs_sceneitem_get_info2(item, &oti);

    obs_sceneitem_crop crop;
    obs_sceneitem_get_crop(item, &crop);

    uint32_t w = obs_source_get_width(model->selectedSource());
    uint32_t h = obs_source_get_height(model->selectedSource());

    double scaledW = w * oti.scale.x;
    double scaledH = h * oti.scale.y;

    auto *pos     = editBox->findChild<EditPosition*>();
    auto *bounds  = editBox->findChild<EditBounds*>();
    auto *cropBox = editBox->findChild<EditCrop*>();

    pos->setPosition(oti.pos.x, oti.pos.y);
    pos->setSize(scaledW, scaledH);
    pos->setSizesEnabled(w != 0 && h != 0);
    pos->setRotation(oti.rot);
    pos->setAlignment(alignToIndex(oti.alignment));

    bounds->setBoundsType(oti.bounds_type);
    bounds->setBoundsSize(oti.bounds.x, oti.bounds.y);
    bounds->setCropToBounds(oti.crop_to_bounds);
    bounds->setAlignment(alignToIndex(oti.bounds_alignment));

    cropBox->setCrop(crop.left, crop.top, crop.right, crop.bottom);
}

void TransformTab::setContainerVisible(QWidget *box, bool wantVisible)
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

void TransformTab::startBoxAnim(QWidget *box, int from, int to, bool hideAtEnd)
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
