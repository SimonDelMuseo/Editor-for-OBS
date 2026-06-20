#include "alignment.hpp"
#include "src/widgets/box.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <obs-module.h>

AlignmentBox::AlignmentBox(QWidget *parent)
    : QWidget(parent)
{
    auto *box = new Box(this);

    /* ============================================================
       LÍNEA 1 — CENTER V / CENTER H
       ============================================================ */
    auto *line1 = new QHBoxLayout();
    line1->setContentsMargins(0, 0, 0, 0);
    line1->setSpacing(10);

    auto *btnCenterV = new QPushButton(box);
    btnCenterV->setObjectName("centerVButton");
    btnCenterV->setFixedHeight(28);
    btnCenterV->setIconSize(QSize(18, 18));
    btnCenterV->setText(obs_module_text("CenterVertically"));
    btnCenterV->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *btnCenterH = new QPushButton(box);
    btnCenterH->setObjectName("centerHButton");
    btnCenterH->setFixedHeight(28);
    btnCenterH->setIconSize(QSize(18, 18));
    btnCenterH->setText(obs_module_text("CenterHorizontally"));
    btnCenterH->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    line1->addWidget(btnCenterV);
    line1->addWidget(btnCenterH);

    /* ============================================================
       LÍNEA 2 — FIT / STRETCH / CENTER TO SCREEN
       ============================================================ */
    auto *line2 = new QHBoxLayout();
    line2->setContentsMargins(0, 0, 0, 0);
    line2->setSpacing(10);

    auto *btnFit = new QPushButton(box);
    btnFit->setObjectName("fitButton");
    btnFit->setFixedHeight(28);
    btnFit->setIconSize(QSize(18, 18));
    btnFit->setText(obs_module_text("FitToScreen"));
    btnFit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *btnStretch = new QPushButton(box);
    btnStretch->setObjectName("stretchButton");
    btnStretch->setFixedHeight(28);
    btnStretch->setIconSize(QSize(18, 18));
    btnStretch->setText(obs_module_text("StretchToScreen"));
    btnStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *btnCenterScreen = new QPushButton(box);
    btnCenterScreen->setObjectName("centerScreenButton");
    btnCenterScreen->setFixedHeight(28);
    btnCenterScreen->setIconSize(QSize(18, 18));
    btnCenterScreen->setText(obs_module_text("CenterToScreen"));
    btnCenterScreen->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    line2->addWidget(btnFit);
    line2->addWidget(btnStretch);
    line2->addWidget(btnCenterScreen);

    /* ============================================================
       INSERTAR EN EL BOX
       ============================================================ */
    auto *inner = qobject_cast<QVBoxLayout*>(box->layout());
    if (inner) {
        inner->addLayout(line1);
        inner->addLayout(line2);
    }

    /* ============================================================
       LAYOUT PRINCIPAL
       ============================================================ */
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->addWidget(box);

    /* ============================================================
       CONEXIONES → SEÑALES
       ============================================================ */
    connect(btnCenterV,      &QPushButton::clicked, this, &AlignmentBox::centerVertical);
    connect(btnCenterH,      &QPushButton::clicked, this, &AlignmentBox::centerHorizontal);

    connect(btnFit,          &QPushButton::clicked, this, &AlignmentBox::fitToScreen);
    connect(btnStretch,      &QPushButton::clicked, this, &AlignmentBox::stretchToScreen);
    connect(btnCenterScreen, &QPushButton::clicked, this, &AlignmentBox::centerToScreen);
}
