#include "copypaste.hpp"
#include "src/widgets/box.hpp"
#include "editor-ui-helpers.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <obs-module.h>

CopyPasteBox::CopyPasteBox(QWidget *parent)
    : QWidget(parent)
{
    auto *box = new Box(this);

    auto *h = new QHBoxLayout();
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(10);

    btnCopy = new QPushButton(box);
    btnCopy->setObjectName("copyButton");
    btnCopy->setFixedHeight(28);
    btnCopy->setIconSize(QSize(18, 18));
    btnCopy->setText(obs_module_text("CopyTransform"));
    btnCopy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editorui::SetButtonIcon(
        btnCopy,
        editorui::GetIconPath("Copy.svg").c_str()
    );

    btnPaste = new QPushButton(box);
    btnPaste->setObjectName("pasteButton");
    btnPaste->setFixedHeight(28);
    btnPaste->setIconSize(QSize(18, 18));
    btnPaste->setText(obs_module_text("PasteTransform"));
    btnPaste->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editorui::SetButtonIcon(
        btnPaste,
        editorui::GetIconPath("Paste.svg").c_str()
    );

    btnReset = new QPushButton(box);
    btnReset->setObjectName("resetButton");
    btnReset->setFixedHeight(28);
    btnReset->setIconSize(QSize(18, 18));
    btnReset->setText(obs_module_text("ResetTransform"));
    btnReset->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editorui::SetButtonIcon(
        btnReset,
        editorui::GetIconPath("Reset.svg").c_str()
    );

    h->addWidget(btnCopy);
    h->addWidget(btnPaste);
    h->addWidget(btnReset);

    auto *inner = qobject_cast<QVBoxLayout*>(box->layout());
    if (inner)
        inner->addLayout(h);

    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->addWidget(box);

    connect(btnCopy,  &QPushButton::clicked, this, &CopyPasteBox::copyTransform);
    connect(btnPaste, &QPushButton::clicked, this, &CopyPasteBox::pasteTransform);
    connect(btnReset, &QPushButton::clicked, this, &CopyPasteBox::resetTransform);
}

void CopyPasteBox::setStatesEnabled(bool copy, bool paste, bool reset)
{
    if (btnCopy)
        btnCopy->setEnabled(copy);
    if (btnPaste)
        btnPaste->setEnabled(paste);
    if (btnReset)
        btnReset->setEnabled(reset);
}
