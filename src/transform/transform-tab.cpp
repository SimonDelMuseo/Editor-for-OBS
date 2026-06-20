/* ────────────────────────────────────────────────
   INCLUDES
   ──────────────────────────────────────────────── */
#include "transform-tab.hpp"
#include "editor-model.hpp"

#include "copypaste.hpp"
#include "quick.hpp"
#include "alignment.hpp"
#include "edit.hpp"

#include "edit-position.hpp"
#include "edit-bounds.hpp"
#include "edit-crop.hpp"

#include <QVBoxLayout>
#include <QLabel>

#include <cmath> // arriba del archivo, si no lo tienes

/* ────────────────────────────────────────────────
   MAPEO REAL DE OBS (NO TOCAR)
   ──────────────────────────────────────────────── */
static const uint32_t indexToAlign[] = {
    OBS_ALIGN_TOP | OBS_ALIGN_LEFT,    // 0
    OBS_ALIGN_TOP,                     // 1
    OBS_ALIGN_TOP | OBS_ALIGN_RIGHT,   // 2
    OBS_ALIGN_LEFT,                    // 3
    OBS_ALIGN_CENTER,                  // 4
    OBS_ALIGN_RIGHT,                   // 5
    OBS_ALIGN_BOTTOM | OBS_ALIGN_LEFT, // 6
    OBS_ALIGN_BOTTOM,                  // 7
    OBS_ALIGN_BOTTOM | OBS_ALIGN_RIGHT // 8
};

/* ────────────────────────────────────────────────
   CONVERTIR MÁSCARA OBS → ÍNDICE 0–8
   ──────────────────────────────────────────────── */
static int alignToIndex(uint32_t align)
{
    for (int i = 0; i < 9; i++)
        if (indexToAlign[i] == align)
            return i;
    return 0;
}

/* ────────────────────────────────────────────────
   CONSTRUCTOR
   ──────────────────────────────────────────────── */
TransformTab::TransformTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    auto *layout = new QVBoxLayout(this);

    copyPasteBox = new CopyPasteBox(this);
    quickBox     = new QuickBox(this);
    alignmentBox = new AlignmentBox(this);
    editBox      = new EditBox(this);

    layout->addWidget(copyPasteBox);
    layout->addWidget(quickBox);
    layout->addWidget(alignmentBox);
    layout->addWidget(editBox);

    layout->addStretch();
    setLayout(layout);

    /* ────────────────────────────────────────────────
       CONEXIONES: COPY/PASTE/RESET
       ──────────────────────────────────────────────── */
        connect(copyPasteBox, &CopyPasteBox::copyTransform,
                this, [this](){ this->model->copyTransform(); });

        connect(copyPasteBox, &CopyPasteBox::pasteTransform,
                this, [this](){ this->model->pasteTransform(); });

        connect(copyPasteBox, &CopyPasteBox::resetTransform,
                this, [this](){ this->model->resetTransform(); });

    /* ────────────────────────────────────────────────
       CONEXIONES: QUICK (ROTATE / FLIP)
       ──────────────────────────────────────────────── */
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

    /* ────────────────────────────────────────────────
       CONEXIONES: ALIGNMENT (FIT / STRETCH / CENTER)
       ──────────────────────────────────────────────── */
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

    /* ────────────────────────────────────────────────
       CONEXIONES: EDIT BOX (POSITION / BOUNDS / CROP)
       ──────────────────────────────────────────────── */

    auto *pos    = editBox->findChild<EditPosition*>();
    auto *bounds = editBox->findChild<EditBounds*>();
    auto *crop   = editBox->findChild<EditCrop*>();

    /* POSITION */
    connect(pos, &EditPosition::posXChanged,
            model, [this, model](double v){
                if (!model->selectedItem()) return;
                obs_transform_info oti;
                obs_sceneitem_get_info2(model->selectedItem(), &oti);
                oti.pos.x = float(v);
                obs_sceneitem_set_info2(model->selectedItem(), &oti);
                model->setSelectedItem(model->selectedItem());
            });

    connect(pos, &EditPosition::posYChanged,
            model, [this, model](double v){
                if (!model->selectedItem()) return;
                obs_transform_info oti;
                obs_sceneitem_get_info2(model->selectedItem(), &oti);
                oti.pos.y = float(v);
                obs_sceneitem_set_info2(model->selectedItem(), &oti);
                model->setSelectedItem(model->selectedItem());
            });

    connect(pos, &EditPosition::widthChanged,
            model, [this, model](double v){
                if (!model->selectedItem()) return;
                vec2 scale;
                obs_sceneitem_get_scale(model->selectedItem(), &scale);

                uint32_t w = obs_source_get_width(model->selectedSource());
                if (w > 0) scale.x = float(v) / float(w);

                obs_sceneitem_set_scale(model->selectedItem(), &scale);
                model->setSelectedItem(model->selectedItem());
            });

    connect(pos, &EditPosition::heightChanged,
            model, [this, model](double v){
                if (!model->selectedItem()) return;
                vec2 scale;
                obs_sceneitem_get_scale(model->selectedItem(), &scale);

                uint32_t h = obs_source_get_height(model->selectedSource());
                if (h > 0) scale.y = float(v) / float(h);

                obs_sceneitem_set_scale(model->selectedItem(), &scale);
                model->setSelectedItem(model->selectedItem());
            });

    connect(pos, &EditPosition::rotationChanged,
            model, [this, model](double v){
                if (!model->selectedItem()) return;
                obs_transform_info oti;
                obs_sceneitem_get_info2(model->selectedItem(), &oti);
                oti.rot = float(v);
                obs_sceneitem_set_info2(model->selectedItem(), &oti);
                model->setSelectedItem(model->selectedItem());
            });

    connect(pos, &EditPosition::alignmentChanged,
            model, [this, model](int index){
                if (!model->selectedItem()) return;
                obs_transform_info oti;
                obs_sceneitem_get_info2(model->selectedItem(), &oti);
                oti.alignment = indexToAlign[index];
                obs_sceneitem_set_info2(model->selectedItem(), &oti);
                model->setSelectedItem(model->selectedItem());
            });

    /* BOUNDS */
connect(bounds, &EditBounds::boundsTypeChanged,
        model, [this, model](int index){
            if (!model->selectedItem()) return;

            obs_sceneitem_t *item = model->selectedItem();
            obs_source_t *source  = model->selectedSource();

            obs_transform_info oti;
            obs_sceneitem_get_info2(item, &oti);

            uint32_t srcW = obs_source_get_width(source);
            uint32_t srcH = obs_source_get_height(source);

            obs_bounds_type lastType = oti.bounds_type;
            obs_bounds_type newType  = (obs_bounds_type)index;

            /* NONE -> otro modo: dar un tamaño de bounds válido
               (usamos el tamaño actual visual de la fuente) */
            if (lastType == OBS_BOUNDS_NONE && newType != OBS_BOUNDS_NONE) {
                vec2 scale;
                obs_sceneitem_get_scale(item, &scale);

                float bw = float(srcW) * std::abs(scale.x);
                float bh = float(srcH) * std::abs(scale.y);

                oti.bounds.x = bw;
                oti.bounds.y = bh;
            }

            /* otro modo -> NONE: quitamos bounds, mantenemos escala tal cual */
            if (lastType != OBS_BOUNDS_NONE && newType == OBS_BOUNDS_NONE) {
                oti.bounds.x = 0.0f;
                oti.bounds.y = 0.0f;
            }

            oti.bounds_type = newType;

            obs_sceneitem_set_info2(item, &oti);
            model->setSelectedItem(item);
        });

        connect(bounds, &EditBounds::boundsWidthChanged,
                model, [this, model](double v){
                if (!model->selectedItem()) return;
                obs_transform_info oti;
                obs_sceneitem_get_info2(model->selectedItem(), &oti);
                oti.bounds.x = float(v);
                obs_sceneitem_set_info2(model->selectedItem(), &oti);
                model->setSelectedItem(model->selectedItem());
                });

        connect(bounds, &EditBounds::boundsHeightChanged,
                model, [this, model](double v){
                if (!model->selectedItem()) return;
                obs_transform_info oti;
                obs_sceneitem_get_info2(model->selectedItem(), &oti);
                oti.bounds.y = float(v);
                obs_sceneitem_set_info2(model->selectedItem(), &oti);
                model->setSelectedItem(model->selectedItem());
                });

        connect(bounds, &EditBounds::cropToBoundsChanged,
                model, [this, model](bool enabled){
                if (!model->selectedItem()) return;
                obs_transform_info oti;
                obs_sceneitem_get_info2(model->selectedItem(), &oti);
                oti.crop_to_bounds = enabled;
                obs_sceneitem_set_info2(model->selectedItem(), &oti);
                model->setSelectedItem(model->selectedItem());
                });

        connect(bounds, &EditBounds::alignmentChanged,
                model, [this, model](int index){
                if (!model->selectedItem()) return;
                obs_transform_info oti;
                obs_sceneitem_get_info2(model->selectedItem(), &oti);
                oti.bounds_alignment = indexToAlign[index];
                obs_sceneitem_set_info2(model->selectedItem(), &oti);
                model->setSelectedItem(model->selectedItem());
                });


    /* CROP */
    connect(crop, &EditCrop::cropLeftChanged,
            model, [this, model](double v){
                if (!model->selectedItem()) return;
                obs_sceneitem_crop c;
                obs_sceneitem_get_crop(model->selectedItem(), &c);
                c.left = int(v);
                obs_sceneitem_set_crop(model->selectedItem(), &c);
                model->setSelectedItem(model->selectedItem());
            });

    connect(crop, &EditCrop::cropTopChanged,
            model, [this, model](double v){
                if (!model->selectedItem()) return;
                obs_sceneitem_crop c;
                obs_sceneitem_get_crop(model->selectedItem(), &c);
                c.top = int(v);
                obs_sceneitem_set_crop(model->selectedItem(), &c);
                model->setSelectedItem(model->selectedItem());
            });

    connect(crop, &EditCrop::cropRightChanged,
            model, [this, model](double v){
                if (!model->selectedItem()) return;
                obs_sceneitem_crop c;
                obs_sceneitem_get_crop(model->selectedItem(), &c);
                c.right = int(v);
                obs_sceneitem_set_crop(model->selectedItem(), &c);
                model->setSelectedItem(model->selectedItem());
            });

    connect(crop, &EditCrop::cropBottomChanged,
            model, [this, model](double v){
                if (!model->selectedItem()) return;
                obs_sceneitem_crop c;
                obs_sceneitem_get_crop(model->selectedItem(), &c);
                c.bottom = int(v);
                obs_sceneitem_set_crop(model->selectedItem(), &c);
                model->setSelectedItem(model->selectedItem());
            });

    /* ────────────────────────────────────────────────
       REFRESH UI
       ──────────────────────────────────────────────── */
    connect(model, &EditorModel::modelUpdated,
            this, &TransformTab::refreshUI);
}

/* ────────────────────────────────────────────────
   REFRESCAR UI
   ──────────────────────────────────────────────── */
void TransformTab::refreshUI()
{
    bool enabled = model->selectedItem() != nullptr;

    copyPasteBox->setEnabled(enabled);
    quickBox->setEnabled(enabled);
    alignmentBox->setEnabled(enabled);
    editBox->setEnabled(enabled);

    if (!enabled)
        return;

        /* ────────────────────────────────────────────────
       DESACTIVAR PANEL SI LA FUENTE ES DE AUDIO
       ──────────────────────────────────────────────── */
    {
        obs_sceneitem_t *item = model->selectedItem();
        obs_source_t *src = obs_sceneitem_get_source(item);
        const char *id = obs_source_get_unversioned_id(src);

        /* Lista real de fuentes de audio en OBS */
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
   

    /* Obtener datos del item */
    obs_sceneitem_t *item = model->selectedItem();
    obs_transform_info oti;
    obs_sceneitem_get_info2(item, &oti);

    obs_sceneitem_crop crop;
    obs_sceneitem_get_crop(item, &crop);

    uint32_t w = obs_source_get_width(model->selectedSource());
    uint32_t h = obs_source_get_height(model->selectedSource());

    double scaledW = w * std::abs(oti.scale.x);
    double scaledH = h * std::abs(oti.scale.y);

    /* Subwidgets */
    auto *pos     = editBox->findChild<EditPosition*>();
    auto *bounds  = editBox->findChild<EditBounds*>();
    auto *cropBox = editBox->findChild<EditCrop*>();

    /* POSITION */
    pos->setPosition(oti.pos.x, oti.pos.y);
    pos->setSize(scaledW, scaledH);
    pos->setRotation(oti.rot);
    pos->setAlignment(alignToIndex(oti.alignment));

    /* BOUNDS */
    bounds->setBoundsType(oti.bounds_type);
    bounds->setBoundsSize(oti.bounds.x, oti.bounds.y);
    bounds->setCropToBounds(oti.crop_to_bounds);
    bounds->setAlignment(alignToIndex(oti.bounds_alignment));

    /* CROP */
    cropBox->setCrop(crop.left, crop.top, crop.right, crop.bottom);
}
