#include "edit-crop.hpp"
#include "src/widgets/e33-spinbox.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
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
    spin->setDecimals(0);
    spin->setRange(0, 100000);
    spin->setSingleStep(1.0);
    spin->setFixedWidth(120);
    spin->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    return spin;
}

EditCrop::EditCrop(QWidget *parent)
    : QWidget(parent)
{
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(0);

    auto *g = new QGridLayout();
    g->setHorizontalSpacing(8);
    g->setVerticalSpacing(2);
    g->setContentsMargins(0, 0, 0, 0);

    g->addWidget(makeSubtitleLabel("Crop", this), 0, 0);

    g->addWidget(makeFieldLabel(obs_module_text("L"), this), 1, 0);
    spinL = makeSpin(this);
    connect(spinL, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditCrop::cropLeftChanged);
    g->addWidget(spinL, 1, 1);

    g->addWidget(makeFieldLabel(obs_module_text("R"), this), 1, 2);
    spinR = makeSpin(this);
    connect(spinR, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditCrop::cropRightChanged);
    g->addWidget(spinR, 1, 3);

    g->addWidget(makeFieldLabel(obs_module_text("Top"), this), 2, 0);
    spinT = makeSpin(this);
    connect(spinT, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditCrop::cropTopChanged);
    g->addWidget(spinT, 2, 1);

    g->addWidget(makeFieldLabel(obs_module_text("Bottom"), this), 2, 2);
    spinB = makeSpin(this);
    connect(spinB, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EditCrop::cropBottomChanged);
    g->addWidget(spinB, 2, 3);

    g->setAlignment(Qt::AlignLeft);
    main->addLayout(g);
}

void EditCrop::setCrop(double left, double top, double right, double bottom)
{
    const bool bl = spinL->blockSignals(true);
    const bool bt = spinT->blockSignals(true);
    const bool br = spinR->blockSignals(true);
    const bool bb = spinB->blockSignals(true);

    spinL->setValue(left);
    spinT->setValue(top);
    spinR->setValue(right);
    spinB->setValue(bottom);

    spinL->blockSignals(bl);
    spinT->blockSignals(bt);
    spinR->blockSignals(br);
    spinB->blockSignals(bb);
}
