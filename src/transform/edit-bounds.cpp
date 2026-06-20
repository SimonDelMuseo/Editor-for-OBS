#include "edit-bounds.hpp"
#include "src/widgets/alignment-grid.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFont>
#include <obs-module.h>

/* Helpers visuales (sin cambios) */
static QLabel *makeBoldLabelTr(const char *key, QWidget *parent)
{
    auto *lbl = new QLabel(obs_module_text(key), parent);
    QFont f = lbl->font();
    f.setBold(true);
    lbl->setFont(f);
    return lbl;
}

static QDoubleSpinBox *makeSpin(QWidget *parent)
{
    auto *spin = new QDoubleSpinBox(parent);
    spin->setSuffix(" px");
    spin->setDecimals(2);
    spin->setRange(0, 99999);
    spin->setFixedWidth(spin->sizeHint().width() - 18);
    spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return spin;
}

EditBounds::EditBounds(QWidget *parent)
    : QWidget(parent)
{
    auto *main = new QHBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(0);

    /* ────────────────────────────────────────────────
       CAJA 1 — BOUNDS
       ──────────────────────────────────────────────── */
    auto *boxLeft = new QVBoxLayout();
    boxLeft->setContentsMargins(0, 0, 0, 0);
    boxLeft->setSpacing(4);

    /* Línea 1: Bounds */
    boxLeft->addWidget(new QLabel(obs_module_text("Bounds"), this));

    /* Línea 2: ComboBox */
    {
        comboType = new QComboBox(this);

        /* Orden REAL de OBS */
        comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.None"));        // 0
        comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.Stretch"));     // 1
        comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.ScaleInner"));  // 2
        comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.ScaleOuter"));  // 3
        comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.ScaleToWidth"));// 4
        comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.ScaleToHeight"));//5
        comboType->addItem(obs_module_text("Basic.TransformWindow.BoundsType.MaxOnly"));     // 6

        connect(comboType, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &EditBounds::boundsTypeChanged);

        boxLeft->addWidget(comboType);
    }

    /* Línea 3: Width + spinbox + checkbox */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lbl = makeBoldLabelTr("Width", this);
        row->addWidget(lbl, 0);

        spinW = makeSpin(this);
        row->addWidget(spinW, 1);

        connect(spinW, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditBounds::boundsWidthChanged);

        chkCrop = new QCheckBox(obs_module_text("CropBound"), this);
        chkCrop->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        row->addWidget(chkCrop, 0);

        connect(chkCrop, &QCheckBox::toggled,
                this, &EditBounds::cropToBoundsChanged);

        boxLeft->addLayout(row);
    }

    /* Línea 4: Height + spinbox + botón fantasma */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lbl = makeBoldLabelTr("Height", this);
        row->addWidget(lbl, 0);

        spinH = makeSpin(this);
        row->addWidget(spinH, 1);

        connect(spinH, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditBounds::boundsHeightChanged);

        auto *ghost = new QPushButton(this);
        ghost->setFlat(true);
        ghost->setEnabled(false);
        ghost->setFocusPolicy(Qt::NoFocus);
        ghost->setStyleSheet("QPushButton { background: transparent; border: none; }");
        ghost->setFixedWidth(82);

        row->addWidget(ghost, 0);

        boxLeft->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       CAJA 2 — ALIGNMENT
       ──────────────────────────────────────────────── */
    auto *boxRight = new QVBoxLayout();
    boxRight->setContentsMargins(6, 0, 0, 0);
    boxRight->setSpacing(4);

    boxRight->addWidget(new QLabel(obs_module_text("Alignment"), this));

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        grid = new AlignmentGrid(this);
        grid->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        row->addWidget(grid, 0);

        connect(grid, &AlignmentGrid::alignmentChanged,
                this, &EditBounds::alignmentChanged);

        row->addStretch(1);
        boxRight->addLayout(row);
    }

    /* Fantasmas */
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

    /* ────────────────────────────────────────────────
       AÑADIR AL LAYOUT PRINCIPAL
       ──────────────────────────────────────────────── */
    main->addLayout(boxLeft);
    main->addLayout(boxRight);

    main->setStretch(0, 1);
    main->setStretch(1, 1);
}

/* ────────────────────────────────────────────────
   MÉTODOS PARA REFRESCAR DESDE EL MODELO
   ──────────────────────────────────────────────── */
void EditBounds::setBoundsType(int index)
{
    blockSignalsFlag = true;
    comboType->setCurrentIndex(index);
    blockSignalsFlag = false;

    bool disableGrid  = false;
    bool disableSpin  = false;
    bool disableCrop  = false;

    switch (index) {
    case 0: /* Automatic / None */
        disableGrid = true;
        disableSpin = true;
        disableCrop = true;
        break;

    case 1: /* Stretch */
        disableGrid = true;
        disableSpin = false;
        disableCrop = true;
        break;

    case 2: /* Fit (ScaleInner) */
        disableGrid = false;
        disableSpin = false;
        disableCrop = true;
        break;

    case 3: /* Cover */
    case 4: /* FillWidth */
    case 5: /* FillHeight */
    case 6: /* MaxOnly */
        disableGrid = false;
        disableSpin = false;
        disableCrop = false;
        break;
    }

    /* ────────────────────────────────────────────────
       APLICAR ESTADO AL GRID (VISUAL + INTERACCIÓN)
       ──────────────────────────────────────────────── */
    grid->setButtonsEnabled(!disableGrid);

    /* Spinboxes */
    spinW->setDisabled(disableSpin);
    spinH->setDisabled(disableSpin);

    /* Crop */
    chkCrop->setDisabled(disableCrop);
}

void EditBounds::setBoundsSize(double w, double h)
{
    blockSignalsFlag = true;
    spinW->setValue(w);
    spinH->setValue(h);
    blockSignalsFlag = false;
}

void EditBounds::setCropToBounds(bool enabled)
{
    blockSignalsFlag = true;
    chkCrop->setChecked(enabled);
    blockSignalsFlag = false;
}

void EditBounds::setAlignment(int index)
{
    blockSignalsFlag = true;
    grid->setSelected(index);
    blockSignalsFlag = false;
}
