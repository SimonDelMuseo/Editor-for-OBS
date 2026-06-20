#include "copypaste.hpp"
#include "src/widgets/box.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <obs-module.h>

CopyPasteBox::CopyPasteBox(QWidget *parent)
    : QWidget(parent)
{
    auto *box = new Box(this);

    /* ────────────────────────────────────────────────
       LAYOUT HORIZONTAL PARA LOS 3 BOTONES
       ──────────────────────────────────────────────── */
    auto *h = new QHBoxLayout();
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(10);

    /* ────────────────────────────────────────────────
       BOTÓN: COPY TRANSFORM
       ──────────────────────────────────────────────── */
    auto *btnCopy = new QPushButton(box);
    btnCopy->setObjectName("copyButton");
    btnCopy->setFixedHeight(28);
    btnCopy->setIconSize(QSize(18, 18));
    btnCopy->setText(obs_module_text("CopyTransform"));
    btnCopy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    /* ────────────────────────────────────────────────
       BOTÓN: PASTE TRANSFORM
       ──────────────────────────────────────────────── */
    auto *btnPaste = new QPushButton(box);
    btnPaste->setObjectName("pasteButton");
    btnPaste->setFixedHeight(28);
    btnPaste->setIconSize(QSize(18, 18));
    btnPaste->setText(obs_module_text("PasteTransform"));
    btnPaste->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    /* ────────────────────────────────────────────────
       BOTÓN: RESET TRANSFORM
       ──────────────────────────────────────────────── */
    auto *btnReset = new QPushButton(box);
    btnReset->setObjectName("resetButton");
    btnReset->setFixedHeight(28);
    btnReset->setIconSize(QSize(18, 18));
    btnReset->setText(obs_module_text("ResetTransform"));
    btnReset->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    /* ────────────────────────────────────────────────
       AÑADIR BOTONES AL LAYOUT
       ──────────────────────────────────────────────── */
    h->addWidget(btnCopy);
    h->addWidget(btnPaste);
    h->addWidget(btnReset);

    /* ────────────────────────────────────────────────
       INSERTAR EN EL BOX
       ──────────────────────────────────────────────── */
    auto *inner = qobject_cast<QVBoxLayout*>(box->layout());
    if (inner)
        inner->addLayout(h);

    /* ────────────────────────────────────────────────
       LAYOUT PRINCIPAL DEL WIDGET
       ──────────────────────────────────────────────── */
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->addWidget(box);

    /* ────────────────────────────────────────────────
       CONEXIONES → SEÑALES
       ──────────────────────────────────────────────── */
    connect(btnCopy,  &QPushButton::clicked, this, &CopyPasteBox::copyTransform);
    connect(btnPaste, &QPushButton::clicked, this, &CopyPasteBox::pasteTransform);
    connect(btnReset, &QPushButton::clicked, this, &CopyPasteBox::resetTransform);
}
