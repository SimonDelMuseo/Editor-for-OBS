#include "quick.hpp"
#include "src/widgets/box.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <obs-module.h>

QuickBox::QuickBox(QWidget *parent)
    : QWidget(parent)
{
    auto *box = new Box(this);

    /* ============================================================
       LÍNEA 1 — FLIPS
       ============================================================ */
    auto *line1 = new QHBoxLayout();
    line1->setContentsMargins(0, 0, 0, 0);
    line1->setSpacing(10);

    auto *btnFlipH = new QPushButton(box);
    btnFlipH->setObjectName("flipHorizontalButton");
    btnFlipH->setFixedHeight(28);
    btnFlipH->setIconSize(QSize(18, 18));
    btnFlipH->setText(obs_module_text("FlipHorizontal"));
    btnFlipH->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto *btnFlipV = new QPushButton(box);
    btnFlipV->setObjectName("flipVerticalButton");
    btnFlipV->setFixedHeight(28);
    btnFlipV->setIconSize(QSize(18, 18));
    btnFlipV->setText(obs_module_text("FlipVertical"));
    btnFlipV->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    line1->addStretch();
    line1->addWidget(btnFlipH);
    line1->addWidget(btnFlipV);
    line1->addStretch();

    /* ============================================================
       LÍNEA 2 — TÍTULO
       ============================================================ */
    auto *title = new QLabel(obs_module_text("RotateTitle"), box);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-weight: bold;");

    /* ============================================================
       LÍNEA 3 — ROTACIONES
       ============================================================ */
    auto *line3 = new QHBoxLayout();
    line3->setContentsMargins(0, 0, 0, 0);
    line3->setSpacing(10);

    auto *btnRotCW = new QPushButton(box);
    btnRotCW->setObjectName("rotate90RightButton");
    btnRotCW->setFixedHeight(28);
    btnRotCW->setIconSize(QSize(18, 18));
    btnRotCW->setText(obs_module_text("Rotate90CW"));
    btnRotCW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *btnRotCCW = new QPushButton(box);
    btnRotCCW->setObjectName("rotate90LeftButton");
    btnRotCCW->setFixedHeight(28);
    btnRotCCW->setIconSize(QSize(18, 18));
    btnRotCCW->setText(obs_module_text("Rotate90CCW"));
    btnRotCCW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *btnRot180 = new QPushButton(box);
    btnRot180->setObjectName("rotate180Button");
    btnRot180->setFixedHeight(28);
    btnRot180->setIconSize(QSize(18, 18));
    btnRot180->setText(obs_module_text("Rotate180"));
    btnRot180->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    line3->addWidget(btnRotCW);
    line3->addWidget(btnRotCCW);
    line3->addWidget(btnRot180);

    /* ============================================================
       INSERTAR TODO EN EL BOX
       ============================================================ */
    auto *inner = qobject_cast<QVBoxLayout*>(box->layout());
    if (inner) {
        inner->addLayout(line1);
        inner->addWidget(title);
        inner->addLayout(line3);
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
    connect(btnFlipH, &QPushButton::clicked, this, &QuickBox::flipHorizontal);
    connect(btnFlipV, &QPushButton::clicked, this, &QuickBox::flipVertical);

    connect(btnRotCW,  &QPushButton::clicked, this, &QuickBox::rotate90CW);
    connect(btnRotCCW, &QPushButton::clicked, this, &QuickBox::rotate90CCW);
    connect(btnRot180, &QPushButton::clicked, this, &QuickBox::rotate180);
}
