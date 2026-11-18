#include "filters-header.hpp"
#include "editor-ui-helpers.hpp"

#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QLabel>

#include <obs-module.h>

FiltersHeader::FiltersHeader(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("E33_FiltersHeader");

    setFrameShape(QFrame::NoFrame);
    setFrameShadow(QFrame::Plain);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(14, 2, 14, 2);
    mainLayout->setSpacing(6);


    btnExpand = new QCheckBox(this);
    btnExpand->setProperty("class", "indicator-expand");
    btnExpand->setCheckable(true);
    btnExpand->setChecked(true);
    btnExpand->setFixedSize(20, 20);

    mainLayout->addWidget(btnExpand);


    btnVisibility = new QCheckBox(this);
    btnVisibility->setProperty("class", "indicator-visibility");
    btnVisibility->setCheckable(true);
    btnVisibility->setChecked(true);
    btnVisibility->setFixedSize(20, 20);

    mainLayout->addWidget(btnVisibility);

    mainLayout->addStretch();


    btnAdd = new QPushButton(this);
    btnAdd->setObjectName("btnAdd");
    btnAdd->setProperty("class", "icon-plus");
    btnAdd->setFixedSize(26, 26);
    btnAdd->setIconSize(QSize(14, 14));
    mainLayout->addWidget(btnAdd);

    btnRemove = new QPushButton(this);
    btnRemove->setProperty("class", "icon-trash");
    btnRemove->setFixedSize(26, 26);
    btnRemove->setIconSize(QSize(14, 14));
    mainLayout->addWidget(btnRemove);

    auto *btnInvisible1 = new QWidget(this);
    btnInvisible1->setFixedSize(10, 10);
    mainLayout->addWidget(btnInvisible1);

    btnEffects = new QToolButton(this);
    btnEffects->setObjectName("effectsButton");
    btnEffects->setText(obs_module_text("EffectFilters"));
    btnEffects->setAutoRaise(true);
    btnEffects->setFixedSize(70, 26);
    mainLayout->addWidget(btnEffects);

    btnAudio = new QToolButton(this);
    btnAudio->setObjectName("audioButton");
    btnAudio->setText(obs_module_text("AudioFilters"));
    btnAudio->setAutoRaise(true);
    btnAudio->setFixedSize(70, 26);
    mainLayout->addWidget(btnAudio);

    auto *btnInvisible2 = new QWidget(this);
    btnInvisible2->setFixedSize(10, 10);
    mainLayout->addWidget(btnInvisible2);

    btnCopy = new QPushButton(this);
    btnCopy->setProperty("class", "icon-copy");
    btnCopy->setFixedSize(26, 26);
    btnCopy->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnCopy,
        editorui::GetIconPath("Copy.svg").c_str()
    );
    mainLayout->addWidget(btnCopy);

    btnPaste = new QPushButton(this);
    btnPaste->setProperty("class", "icon-paste");
    btnPaste->setFixedSize(26, 26);
    btnPaste->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnPaste,
        editorui::GetIconPath("Paste.svg").c_str()
    );
    mainLayout->addWidget(btnPaste);

    mainLayout->addStretch();


    btnSelectAll = new QCheckBox(this);
    btnSelectAll->setObjectName("filterSelectCheck");
    btnSelectAll->setFixedSize(20, 20);
    mainLayout->addWidget(btnSelectAll);

    btnPopoutAll = new QPushButton(this);
    btnPopoutAll->setFlat(true);
    btnPopoutAll->setFixedSize(20, 20);
    btnPopoutAll->setObjectName("popoutButton");
    btnPopoutAll->setIconSize(QSize(14, 14));
    editorui::SetButtonIcon(
        btnPopoutAll,
        editorui::GetIconPath("Popout.svg").c_str()
    );
    btnPopoutAll->setStyleSheet(
        "QPushButton { background: transparent; padding: 0; border: none; }"
    );
    mainLayout->addWidget(btnPopoutAll);


    connect(btnExpand, &QCheckBox::clicked, this, [this](bool checked) {       
        emit expandCollapseAllRequested(checked);
    });


    connect(btnVisibility, &QCheckBox::clicked, this, [this](bool checked) {
        emit visibilityAllRequested(checked);
    });
    

    connect(btnAdd, &QPushButton::clicked, this, [this]() {
        emit addFilterRequested();
    });


    connect(btnRemove, &QPushButton::clicked, this, [this]() {
        emit removeFilterRequested();
    });


    connect(btnEffects, &QToolButton::clicked, this, [this]() {
        if (currentCategory == FilterCategory::Effect)
            return;
        setActiveCategory(FilterCategory::Effect);
        emit categoryChanged(FilterCategory::Effect);
    });

    connect(btnAudio, &QToolButton::clicked, this, [this]() {
        if (currentCategory == FilterCategory::Audio)
            return;
        setActiveCategory(FilterCategory::Audio);
        emit categoryChanged(FilterCategory::Audio);
    });

    connect(btnCopy, &QPushButton::clicked, this, [this]() {
        emit copyRequested();
    });

    connect(btnPaste, &QPushButton::clicked, this, [this]() {
        emit pasteRequested();
    });


    connect(btnSelectAll, &QCheckBox::clicked, this, [this](bool checked) {
        emit selectAllRequested(checked);
    });


    connect(btnPopoutAll, &QPushButton::clicked, this, [this]() {
        emit popoutAllRequested();
    });

    setActiveCategory(FilterCategory::Effect);
}

void FiltersHeader::setActiveCategory(FilterCategory category)
{
    currentCategory = category;

    if (btnEffects->isEnabled())
        btnEffects->setStyleSheet(
            (category == FilterCategory::Effect)
                ? "border: 2px solid #FF4444;"
                : "border: 2px solid #16c416;");
    else
        btnEffects->setStyleSheet("border: 1px solid palette(button);");

    if (btnAudio->isEnabled())
        btnAudio->setStyleSheet(
            (category == FilterCategory::Audio)
                ? "border: 2px solid #FF4444;"
                : "border: 2px solid #16c416;");
    else
        btnAudio->setStyleSheet("border: 1px solid palette(button);");
}


void FiltersHeader::updateAudioButtonState(bool enabled)
{
    btnAudio->setEnabled(enabled);

    if (!enabled) {
        btnAudio->setStyleSheet("border: 1px solid palette(button);");
        return;
    }

    setActiveCategory(currentCategory);
}

void FiltersHeader::disableAudioCategory()
{
    if (currentCategory == FilterCategory::Audio) {
        setActiveCategory(FilterCategory::Effect);
        emit categoryChanged(FilterCategory::Effect);
    }

    btnAudio->setEnabled(false);

    btnAudio->setStyleSheet("border: 1px solid palette(button);");
}

void FiltersHeader::setStatesEnabled(bool add, bool remove, bool copy,
                                     bool paste, bool effects, bool audio)
{
    if (btnAdd)
        btnAdd->setEnabled(add);
    if (btnRemove)
        btnRemove->setEnabled(remove);
    if (btnCopy)
        btnCopy->setEnabled(copy);
    if (btnPaste)
        btnPaste->setEnabled(paste);
    if (btnEffects) {
        btnEffects->setEnabled(effects);
        if (!effects)
            btnEffects->setStyleSheet("border: 1px solid palette(button);");
        else
            btnEffects->setStyleSheet(
                (currentCategory == FilterCategory::Effect)
                    ? "border: 2px solid #FF4444;"
                    : "border: 2px solid #16c416;");
    }

    updateAudioButtonState(audio);
}

