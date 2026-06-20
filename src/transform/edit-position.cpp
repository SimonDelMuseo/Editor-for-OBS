#include "edit-position.hpp"
#include "src/widgets/alignment-grid.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QFont>
#include <obs-module.h>

/* Helpers visuales (sin cambios) */
static QLabel *makeBoldLabel(const char *text, QWidget *parent)
{
    auto *lbl = new QLabel(text, parent);
    QFont f = lbl->font();
    f.setBold(true);
    lbl->setFont(f);
    return lbl;
}

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
    spin->setRange(-99999, 99999);
    spin->setFixedWidth(spin->sizeHint().width() - 18);
    spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return spin;
}

static QDoubleSpinBox *makeSpinSize(QWidget *parent)
{
    auto *spin = new QDoubleSpinBox(parent);
    spin->setSuffix(" px");
    spin->setDecimals(2);
    spin->setRange(0, 99999);
    spin->setFixedWidth(spin->sizeHint().width() - 18);
    spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return spin;
}

static QDoubleSpinBox *makeSpinRot(QWidget *parent)
{
    auto *spin = new QDoubleSpinBox(parent);
    spin->setSuffix(" º");
    spin->setDecimals(2);
    spin->setRange(-360, 360);
    spin->setFixedWidth(spin->sizeHint().width() - 18);
    spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return spin;
}

EditPosition::EditPosition(QWidget *parent)
    : QWidget(parent)
{
    auto *main = new QHBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(0);

    /* ────────────────────────────────────────────────
       CAJA 1 — POSITION
       ──────────────────────────────────────────────── */
    auto *boxPos = new QVBoxLayout();
    boxPos->setContentsMargins(0, 0, 0, 0);
    boxPos->setSpacing(4);

    boxPos->addWidget(new QLabel(obs_module_text("Position"), this));

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lblX = makeBoldLabel("X", this);
        lblX->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        row->addWidget(lblX, 0);

        spinX = makeSpin(this);
        spinX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        row->addWidget(spinX, 1);

        connect(spinX, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditPosition::posXChanged);

        boxPos->addLayout(row);
    }

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lblY = makeBoldLabel("Y", this);
        lblY->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        row->addWidget(lblY, 0);

        spinY = makeSpin(this);
        spinY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        row->addWidget(spinY, 1);

        connect(spinY, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditPosition::posYChanged);

        boxPos->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       CAJA 2 — SIZE
       ──────────────────────────────────────────────── */
    auto *boxSize = new QVBoxLayout();
    boxSize->setContentsMargins(6, 0, 6, 0);
    boxSize->setSpacing(4);

    boxSize->addWidget(new QLabel(obs_module_text("Size"), this));

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lblW = makeBoldLabelTr("Width", this);
        lblW->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        row->addWidget(lblW, 0);

        spinW = makeSpinSize(this);
        spinW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        row->addWidget(spinW, 1);

        connect(spinW, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditPosition::widthChanged);

        boxSize->addLayout(row);
    }

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lblH = makeBoldLabelTr("Height", this);
        lblH->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        row->addWidget(lblH, 0);

        spinH = makeSpinSize(this);
        spinH->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        row->addWidget(spinH, 1);

        connect(spinH, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditPosition::heightChanged);

        boxSize->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       CAJA 3 — ROTATION + ALIGNMENT
       ──────────────────────────────────────────────── */
    auto *boxRot = new QVBoxLayout();
    boxRot->setContentsMargins(0, 0, 0, 0);
    boxRot->setSpacing(4);

    boxRot->addWidget(new QLabel(obs_module_text("Rotation"), this));

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        spinRot = makeSpinRot(this);
        row->addWidget(spinRot);

        connect(spinRot, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditPosition::rotationChanged);

        boxRot->addLayout(row);
    }

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        auto *lbl = new QLabel(obs_module_text("Alignment"), this);
        lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        row->addWidget(lbl);

        grid = new AlignmentGrid(this);
        grid->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        row->addWidget(grid);

        connect(grid, &AlignmentGrid::alignmentChanged,
                this, &EditPosition::alignmentChanged);

        boxRot->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       AÑADIR LAS 3 CAJAS
       ──────────────────────────────────────────────── */
    main->addLayout(boxPos);
    main->addLayout(boxSize);
    main->addLayout(boxRot);

    main->setStretch(0, 1);
    main->setStretch(1, 1);
    main->setStretch(2, 1);
}

/* ────────────────────────────────────────────────
   MÉTODOS PARA REFRESCAR DESDE EL MODELO
   (bloquean señales para evitar loops)
   ──────────────────────────────────────────────── */
void EditPosition::setPosition(double x, double y)
{
    blockSignalsFlag = true;
    spinX->setValue(x);
    spinY->setValue(y);
    blockSignalsFlag = false;
}

void EditPosition::setSize(double w, double h)
{
    blockSignalsFlag = true;
    spinW->setValue(w);
    spinH->setValue(h);
    blockSignalsFlag = false;
}

void EditPosition::setRotation(double rot)
{
    blockSignalsFlag = true;
    spinRot->setValue(rot);
    blockSignalsFlag = false;
}

void EditPosition::setAlignment(int index)
{
    blockSignalsFlag = true;
    grid->setSelected(index);
    blockSignalsFlag = false;
}
