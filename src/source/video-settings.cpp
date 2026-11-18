#include "src/source/video-settings.hpp"
#include "editor-model.hpp"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <obs.h>
#include <obs-module.h>

VideoSettings::VideoSettings(QWidget *parent)
    : Box(parent)
{
    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->setSpacing(6);

    QWidget *row = new QWidget(this);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(6);

    auto *title = new QLabel(QString("<b>%1</b>").arg(obs_module_text("Deinterlacing")), this);
    title->setTextFormat(Qt::RichText);
    rowLayout->addWidget(title);

    deinterlaceCombo = new QComboBox(this);
    deinterlaceCombo->setEditable(false);
    deinterlaceCombo->setInsertPolicy(QComboBox::NoInsert);
    deinterlaceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    deinterlaceCombo->addItem(obs_module_text("Deinterlace_Disable"),
                              OBS_DEINTERLACE_MODE_DISABLE);
    deinterlaceCombo->addItem(obs_module_text("Deinterlace_Discard"),
                              OBS_DEINTERLACE_MODE_DISCARD);
    deinterlaceCombo->addItem("Retro",     OBS_DEINTERLACE_MODE_RETRO);
    deinterlaceCombo->addItem("Blend",     OBS_DEINTERLACE_MODE_BLEND);
    deinterlaceCombo->addItem("Blend 2x",  OBS_DEINTERLACE_MODE_BLEND_2X);
    deinterlaceCombo->addItem("Linear",    OBS_DEINTERLACE_MODE_LINEAR);
    deinterlaceCombo->addItem("Linear 2x", OBS_DEINTERLACE_MODE_LINEAR_2X);
    deinterlaceCombo->addItem("Yadif",     OBS_DEINTERLACE_MODE_YADIF);
    deinterlaceCombo->addItem("Yadif 2x",  OBS_DEINTERLACE_MODE_YADIF_2X);

    rowLayout->addWidget(deinterlaceCombo);
    mainLayout->addWidget(row);

    QWidget *fieldRow = new QWidget(this);
    auto *fieldLayout = new QHBoxLayout(fieldRow);
    fieldLayout->setContentsMargins(0, 0, 0, 0);
    fieldLayout->setSpacing(12);

    fieldLayout->addStretch();

    auto *topField = new QRadioButton(obs_module_text("TopFieldFirst"), this);
    auto *bottomField = new QRadioButton(obs_module_text("BottomFieldFirst"), this);

    auto *group = new QButtonGroup(this);
    group->addButton(topField);
    group->addButton(bottomField);

    topField->setChecked(true);

    topFieldBtn = topField;
    bottomFieldBtn = bottomField;

    fieldLayout->addWidget(topField);
    fieldLayout->addWidget(bottomField);

    mainLayout->addWidget(fieldRow);

    auto updateFieldVisibility = [fieldRow, this]() {
        bool enabled = (deinterlaceCombo->currentData().toInt() != OBS_DEINTERLACE_MODE_DISABLE);
        fieldRow->setVisible(enabled);
    };

    connect(deinterlaceCombo, &QComboBox::currentIndexChanged, this,
            [this, updateFieldVisibility](int index) {

                if (model) {
                    obs_source_t *src = model->selectedSource();
                    if (src) {
                        int mode = deinterlaceCombo->itemData(index).toInt();
                        obs_deinterlace_mode old =
                            obs_source_get_deinterlace_mode(src);

                        if ((int)old != mode) {
                            obs_source_set_deinterlace_mode(
                                src, (obs_deinterlace_mode)mode);
                            model->pushSourcePropUndo("UndoDeinterlace", src,
                                                      "deinterlace_mode",
                                                      old, mode, false);
                        }
                    }
                }

                updateFieldVisibility();
            });

    connect(topField, &QRadioButton::toggled, this,
            [this](bool checked) {
                if (!checked)
                    return;

                if (model) {
                    obs_source_t *src = model->selectedSource();
                    if (!src)
                        return;

                    obs_deinterlace_field_order old =
                        obs_source_get_deinterlace_field_order(src);
                    if (old == OBS_DEINTERLACE_FIELD_ORDER_TOP)
                        return;

                    obs_source_set_deinterlace_field_order(
                        src, OBS_DEINTERLACE_FIELD_ORDER_TOP);
                    model->pushSourcePropUndo(
                        "UndoDeinterlaceOrder", src, "deinterlace_order",
                        old, OBS_DEINTERLACE_FIELD_ORDER_TOP, false);
                }
            });

    connect(bottomField, &QRadioButton::toggled, this,
            [this](bool checked) {
                if (!checked)
                    return;

                if (model) {
                    obs_source_t *src = model->selectedSource();
                    if (!src)
                        return;

                    obs_deinterlace_field_order old =
                        obs_source_get_deinterlace_field_order(src);
                    if (old == OBS_DEINTERLACE_FIELD_ORDER_BOTTOM)
                        return;

                    obs_source_set_deinterlace_field_order(
                        src, OBS_DEINTERLACE_FIELD_ORDER_BOTTOM);
                    model->pushSourcePropUndo(
                        "UndoDeinterlaceOrder", src, "deinterlace_order",
                        old, OBS_DEINTERLACE_FIELD_ORDER_BOTTOM, false);
                }
            });

    fieldRow->setVisible(false);
}

void VideoSettings::setModel(EditorModel *m)
{
    model = m;

    connect(model, &EditorModel::modelUpdated, this,
            [this]() {
                if (!model)
                    return;

                obs_source_t *src = model->selectedSource();
                if (!src)
                    return;

                obs_deinterlace_mode mode = obs_source_get_deinterlace_mode(src);
                int index = deinterlaceCombo->findData((int)mode);
                if (index >= 0)
                    deinterlaceCombo->setCurrentIndex(index);

                obs_deinterlace_field_order order =
                    obs_source_get_deinterlace_field_order(src);
                if (bottomFieldBtn)
                    bottomFieldBtn->setChecked(
                        order == OBS_DEINTERLACE_FIELD_ORDER_BOTTOM);
                if (topFieldBtn)
                    topFieldBtn->setChecked(
                        order != OBS_DEINTERLACE_FIELD_ORDER_BOTTOM);
            });
}
