#include "edit-bounds.hpp"
#include "src/widgets/alignment-grid.hpp"
#include "src/widgets/e33-spinbox.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFont>
#include <obs-module.h>


static QLabel *makeSubtitleLabel(const char *key, QWidget *parent)
{
    auto *lbl = new QLabel(obs_module_text(key), parent);
    lbl->setProperty("class", "subtitle");
    return lbl;
}

static QLabel *makeFieldLabel(const QString &text, QWidget *parent)
{
    auto *lbl = new QLabel(text, parent);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return lbl;
}

static QDoubleSpinBox *makeSpin(QWidget *parent)
{
    auto *spin = new E33DoubleSpinBox(parent);
    spin->setSuffix(" px");
    spin->setDecimals(2);
    spin->setRange(1, 90001);
    spin->setSingleStep(1.0);
    return spin;
}

EditBounds::EditBounds(QWidget *parent)
    : QWidget(parent)
{
    auto *main = new QHBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(8);

    auto *boxLeft = new QGridLayout();
    boxLeft->setHorizontalSpacing(8);
    boxLeft->setVerticalSpacing(2);
    boxLeft->setContentsMargins(0, 0, 0, 0);

    boxLeft->addWidget(makeSubtitleLabel("Bounds", this), 0, 0, 1, 3);

    comboType = new QComboBox(this);

    comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.None"));
    comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.Stretch"));
    comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.ScaleInner"));
    comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.ScaleOuter"));
    comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.ScaleToWidth"));
    comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.ScaleToHeight"));
    comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.MaxOnly"));

    connect(comboType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditBounds::boundsTypeChanged);

    boxLeft->addWidget(comboType, 1, 0, 1, 3);

    {
        boxLeft->addWidget(makeFieldLabel(obs_module_text("Width"), this), 2, 0);

        spinW = makeSpin(this);
        spinW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(spinW, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditBounds::boundsWidthChanged);
        boxLeft->addWidget(spinW, 2, 1);

        chkCrop = new QCheckBox(obs_module_text("CropBound"), this);
        chkCrop->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(chkCrop, &QCheckBox::toggled,
                this, &EditBounds::cropToBoundsChanged);
        boxLeft->addWidget(chkCrop, 2, 2);
    }

    {
        boxLeft->addWidget(makeFieldLabel(obs_module_text("Height"), this), 3, 0);

        spinH = makeSpin(this);
        spinH->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(spinH, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditBounds::boundsHeightChanged);
        boxLeft->addWidget(spinH, 3, 1);
    }


    auto *boxRight = new QVBoxLayout();
    boxRight->setContentsMargins(0, 0, 0, 0);
    boxRight->setSpacing(4);

    boxRight->addWidget(makeSubtitleLabel("Alignment", this));

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        grid = new AlignmentSelector(this);
        grid->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        row->addWidget(grid, 0);

        connect(grid, &AlignmentSelector::alignmentChanged,
                this, &EditBounds::alignmentChanged);

        row->addStretch(1);
        boxRight->addLayout(row);
    }

    {
        auto *ghost = new QPushButton(this);
        ghost->setFlat(true);
        ghost->setEnabled(false);
        ghost->setFocusPolicy(Qt::NoFocus);
        ghost->setStyleSheet("QPushButton { background: transparent; border: none; }");
        ghost->setFixedHeight(24);
        boxRight->addWidget(ghost);
    }
    {
        auto *ghost = new QPushButton(this);
        ghost->setFlat(true);
        ghost->setEnabled(false);
        ghost->setFocusPolicy(Qt::NoFocus);
        ghost->setStyleSheet("QPushButton { background: transparent; border: none; }");
        ghost->setFixedHeight(24);
        boxRight->addWidget(ghost);
    }

    main->addLayout(boxLeft);
    main->addLayout(boxRight);

    main->setStretch(0, 2);
    main->setStretch(1, 1);
}

void EditBounds::setBoundsType(int index)
{
    const bool bc = comboType->blockSignals(true);
    comboType->setCurrentIndex(index);
    comboType->blockSignals(bc);

    bool disableGrid  = false;
    bool disableSpin  = false;
    bool disableCrop  = false;

    switch (index) {
    case 0:
        disableGrid = true;
        disableSpin = true;
        disableCrop = true;
        break;

    case 1:
        disableGrid = true;
        disableSpin = false;
        disableCrop = true;
        break;

    case 2:
        disableGrid = false;
        disableSpin = false;
        disableCrop = true;
        break;

    case 3:
    case 4:
    case 5:
        disableGrid = false;
        disableSpin = false;
        disableCrop = false;
        break;

    case 6:
        disableGrid = false;
        disableSpin = false;
        disableCrop = true;
        break;
    }

    grid->setButtonsEnabled(!disableGrid);

    spinW->setDisabled(disableSpin);
    spinH->setDisabled(disableSpin);

    chkCrop->setDisabled(disableCrop);
}

void EditBounds::setBoundsSize(double w, double h)
{
    const bool bw = spinW->blockSignals(true);
    const bool bh = spinH->blockSignals(true);

    spinW->setValue(w);
    spinH->setValue(h);

    spinW->blockSignals(bw);
    spinH->blockSignals(bh);
}

void EditBounds::setCropToBounds(bool enabled)
{
    const bool bk = chkCrop->blockSignals(true);
    chkCrop->setChecked(enabled);
    chkCrop->blockSignals(bk);
}

void EditBounds::setAlignment(int index)
{
    const bool bg = grid->blockSignals(true);
    grid->setSelected(index);
    grid->blockSignals(bg);
}
