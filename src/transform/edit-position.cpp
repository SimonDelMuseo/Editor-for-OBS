#include "edit-position.hpp"
#include "src/widgets/alignment-grid.hpp"
#include "src/widgets/e33-spinbox.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QFont>
#include <obs-module.h>

static QLabel *makeSubtitleLabel(const char *key, QWidget *parent)
{
    auto *lbl = new QLabel(obs_module_text(key), parent);
    lbl->setProperty("class", "subtitle");
    return lbl;
}

static QDoubleSpinBox *makeSpin(QWidget *parent)
{
    auto *spin = new E33DoubleSpinBox(parent);
    spin->setSuffix(" px");
    spin->setDecimals(2);
    spin->setRange(-90001, 90001);
    spin->setSingleStep(1.0);
    return spin;
}

static QDoubleSpinBox *makeSpinSize(QWidget *parent)
{
    auto *spin = new E33DoubleSpinBox(parent);
    spin->setSuffix(" px");
    spin->setDecimals(2);
    spin->setRange(-90001, 90001);
    spin->setSingleStep(1.0);
    return spin;
}

static QDoubleSpinBox *makeSpinRot(QWidget *parent)
{
    auto *spin = new E33DoubleSpinBox(parent);
    spin->setSuffix(QString::fromUtf8("\xC2\xB0"));
    spin->setDecimals(2);
    spin->setRange(-360, 360);
    spin->setSingleStep(0.1);
    spin->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    return spin;
}

static QLabel *makeFieldLabel(const QString &text, QWidget *parent)
{
    auto *lbl = new QLabel(text, parent);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return lbl;
}

EditPosition::EditPosition(QWidget *parent)
    : QWidget(parent)
{
    auto *main = new QHBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(8);

    auto *boxPos = new QGridLayout();
    boxPos->setHorizontalSpacing(8);
    boxPos->setVerticalSpacing(2);
    boxPos->setContentsMargins(0, 0, 0, 0);
    boxPos->setColumnStretch(1, 1);

    boxPos->addWidget(makeSubtitleLabel("Position", this), 0, 0, 1, 2);

    boxPos->addWidget(makeFieldLabel("X", this), 1, 0);
    spinX = makeSpin(this);
    spinX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(spinX, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditPosition::posXChanged);
    boxPos->addWidget(spinX, 1, 1);

    boxPos->addWidget(makeFieldLabel("Y", this), 2, 0);
    spinY = makeSpin(this);
    spinY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(spinY, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditPosition::posYChanged);
    boxPos->addWidget(spinY, 2, 1);


    auto *boxSize = new QGridLayout();
    boxSize->setHorizontalSpacing(8);
    boxSize->setVerticalSpacing(2);
    boxSize->setContentsMargins(0, 0, 0, 0);
    boxSize->setColumnStretch(1, 1);

    boxSize->addWidget(makeSubtitleLabel("Size", this), 0, 0, 1, 2);

    boxSize->addWidget(makeFieldLabel(obs_module_text("Width"), this), 1, 0);
    spinW = makeSpinSize(this);
    spinW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(spinW, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditPosition::widthChanged);
    boxSize->addWidget(spinW, 1, 1);

    boxSize->addWidget(makeFieldLabel(obs_module_text("Height"), this), 2, 0);
    spinH = makeSpinSize(this);
    spinH->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(spinH, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditPosition::heightChanged);
    boxSize->addWidget(spinH, 2, 1);

    auto *boxRot = new QVBoxLayout();
    boxRot->setContentsMargins(0, 0, 0, 0);
    boxRot->setSpacing(2);

    boxRot->addWidget(makeSubtitleLabel("Rotation", this));

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(4);

        spinRot = makeSpinRot(this);
        row->addWidget(spinRot);
        row->addStretch(1);

        connect(spinRot, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &EditPosition::rotationChanged);

        boxRot->addLayout(row);
    }

    {
        auto *row = new QHBoxLayout();
        row->setSpacing(8);

        row->addWidget(makeSubtitleLabel("Alignment", this));

        grid = new AlignmentSelector(this);
        grid->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        row->addWidget(grid);
        row->addStretch(1);

        connect(grid, &AlignmentSelector::alignmentChanged,
                this, &EditPosition::alignmentChanged);

        boxRot->addLayout(row);
    }

    main->addLayout(boxPos);
    main->addLayout(boxSize);
    main->addLayout(boxRot);

    main->setAlignment(boxPos,  Qt::AlignTop);
    main->setAlignment(boxSize, Qt::AlignTop);
    main->setAlignment(boxRot,  Qt::AlignTop);

    main->setStretch(0, 1);
    main->setStretch(1, 1);
    main->setStretch(2, 1);
}

void EditPosition::setPosition(double x, double y)
{
    const bool bx = spinX->blockSignals(true);
    const bool by = spinY->blockSignals(true);

    spinX->setValue(x);
    spinY->setValue(y);

    spinX->blockSignals(bx);
    spinY->blockSignals(by);
}

void EditPosition::setSize(double w, double h)
{
    const bool bw = spinW->blockSignals(true);
    const bool bh = spinH->blockSignals(true);

    spinW->setValue(w);
    spinH->setValue(h);

    spinW->blockSignals(bw);
    spinH->blockSignals(bh);
}

void EditPosition::setRotation(double rot)
{
    const bool br = spinRot->blockSignals(true);

    spinRot->setValue(rot);

    spinRot->blockSignals(br);
}

void EditPosition::setAlignment(int index)
{
    const bool bg = grid->blockSignals(true);

    grid->setSelected(index);

    grid->blockSignals(bg);
}

void EditPosition::setSizesEnabled(bool enabled)
{
    spinW->setEnabled(enabled);
    spinH->setEnabled(enabled);
}
