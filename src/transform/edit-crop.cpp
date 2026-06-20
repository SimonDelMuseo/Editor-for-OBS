#include "edit-crop.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QFont>
#include <obs-module.h>

/* Helpers visuales */
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

EditCrop::EditCrop(QWidget *parent)
    : QWidget(parent)
{
    auto *main = new QHBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(0);

    /* ────────────────────────────────────────────────
       CAJA 1 — LEFT / TOP
       ──────────────────────────────────────────────── */
    auto *boxLeft = new QVBoxLayout();
    boxLeft->setContentsMargins(0, 0, 0, 0);
    boxLeft->setSpacing(4);

    boxLeft->addWidget(new QLabel(obs_module_text("Crop"), this));

    /* LEFT */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lbl = makeBoldLabelTr("L", this);
        row->addWidget(lbl, 0);

        spinL = makeSpin(this);
        row->addWidget(spinL, 1);

        connect(spinL, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditCrop::cropLeftChanged);

        boxLeft->addLayout(row);
    }

    /* TOP */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lbl = makeBoldLabelTr("Top", this);
        row->addWidget(lbl, 0);

        spinT = makeSpin(this);
        row->addWidget(spinT, 1);

        connect(spinT, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditCrop::cropTopChanged);

        boxLeft->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       CAJA 2 — RIGHT / BOTTOM
       ──────────────────────────────────────────────── */
    auto *boxRight = new QVBoxLayout();
    boxRight->setContentsMargins(6, 0, 0, 0);
    boxRight->setSpacing(4);

    /* Línea fantasma */
    {
        auto *lbl = new QLabel(obs_module_text("Crop"), this);
        lbl->setStyleSheet("QLabel { color: transparent; }");
        boxRight->addWidget(lbl);
    }

    /* RIGHT */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lbl = makeBoldLabelTr("R", this);
        row->addWidget(lbl, 0);

        spinR = makeSpin(this);
        row->addWidget(spinR, 1);

        connect(spinR, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditCrop::cropRightChanged);

        boxRight->addLayout(row);
    }

    /* BOTTOM */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lbl = makeBoldLabelTr("Bottom", this);
        row->addWidget(lbl, 0);

        spinB = makeSpin(this);
        row->addWidget(spinB, 1);

        connect(spinB, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditCrop::cropBottomChanged);

        boxRight->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       CAJA 3 — VACÍA
       ──────────────────────────────────────────────── */
    auto *boxEmpty = new QVBoxLayout();
    boxEmpty->setContentsMargins(6, 0, 0, 0);
    boxEmpty->setSpacing(0);

    /* ────────────────────────────────────────────────
       AÑADIR TODO
       ──────────────────────────────────────────────── */
    main->addLayout(boxLeft);
    main->addLayout(boxRight);
    main->addLayout(boxEmpty);

    main->setStretch(0, 1);
    main->setStretch(1, 1);
    main->setStretch(2, 1);
}

/* ────────────────────────────────────────────────
   REFRESCAR DESDE EL MODELO
   ──────────────────────────────────────────────── */
void EditCrop::setCrop(double left, double top, double right, double bottom)
{
    blockSignalsFlag = true;

    spinL->setValue(left);
    spinT->setValue(top);
    spinR->setValue(right);
    spinB->setValue(bottom);

    blockSignalsFlag = false;
}
