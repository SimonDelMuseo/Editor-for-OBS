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

    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

    // ============================================================
    // LAYOUT PRINCIPAL (UNA SOLA LÍNEA)
    // ============================================================
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(14, 2, 14, 2);
    mainLayout->setSpacing(6);

    // ============================================================
    // IZQUIERDA — expandir / colapsar
    // ============================================================

    btnExpand = new QCheckBox(this);
    btnExpand->setProperty("class", "indicator-expand");
    btnExpand->setCheckable(true);
    btnExpand->setChecked(true);
    btnExpand->setFixedSize(20, 20);

    mainLayout->addWidget(btnExpand);

    // ============================================================
    // IZQUIERDA — visibility (mostrar / ocultar)
    // ============================================================

    btnVisibility = new QCheckBox(this);
    btnVisibility->setProperty("class", "indicator-visibility");
    btnVisibility->setCheckable(true);
    btnVisibility->setChecked(true);
    btnVisibility->setFixedSize(20, 20);

    mainLayout->addWidget(btnVisibility);

    // ============================================================
    // CENTRO — stretch izquierdo
    // ============================================================
    mainLayout->addStretch();

    // ============================================================
    // CENTRO — botones principales
    // ============================================================

    // Añadir (icono OBS)
    btnAdd = new QPushButton(this);
    btnAdd->setObjectName("btnAdd");
    btnAdd->setProperty("class", "icon-plus");
    btnAdd->setFixedSize(26, 26);
    btnAdd->setIconSize(QSize(14, 14));
    mainLayout->addWidget(btnAdd);

    // Eliminar (icono OBS)
    btnRemove = new QPushButton(this);
    btnRemove->setProperty("class", "icon-trash");
    btnRemove->setFixedSize(26, 26);
    btnRemove->setIconSize(QSize(14, 14));
    mainLayout->addWidget(btnRemove);

    // Botón invisible (espaciador visual)
    auto *btnInvisible1 = new QWidget(this);
    btnInvisible1->setFixedSize(10, 10);
    mainLayout->addWidget(btnInvisible1);

    // Effect (texto traducible + preparado para borde verde/rojo)
    btnEffects = new QToolButton(this);
    btnEffects->setObjectName("effectsButton");
    btnEffects->setText(obs_module_text("EffectFilters"));
    btnEffects->setAutoRaise(true);
    btnEffects->setFixedSize(70, 26);
    mainLayout->addWidget(btnEffects);

    // Audio (texto traducible + preparado para borde verde/rojo)
    btnAudio = new QToolButton(this);
    btnAudio->setObjectName("audioButton");
    btnAudio->setText(obs_module_text("AudioFilters"));
    btnAudio->setAutoRaise(true);
    btnAudio->setFixedSize(70, 26);
    mainLayout->addWidget(btnAudio);

    // Botón invisible (espaciador visual)
    auto *btnInvisible2 = new QWidget(this);
    btnInvisible2->setFixedSize(10, 10);
    mainLayout->addWidget(btnInvisible2);

    // Copiar (icono OBS)
    btnCopy = new QPushButton(this);
    btnCopy->setProperty("class", "icon-copy");
    btnCopy->setFixedSize(26, 26);
    btnCopy->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnCopy,
        editorui::GetIconPath("Copy.svg").c_str()
    );
    mainLayout->addWidget(btnCopy);

    // Pegar (icono OBS)
    btnPaste = new QPushButton(this);
    btnPaste->setProperty("class", "icon-paste");
    btnPaste->setFixedSize(26, 26);
    btnPaste->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnPaste,
        editorui::GetIconPath("Paste.svg").c_str()
    );
    mainLayout->addWidget(btnPaste);

    // ============================================================
    // CENTRO — stretch derecho
    // ============================================================
    mainLayout->addStretch();

    // ============================================================
    // DERECHA — checkbox + popout
    // ============================================================

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

    // ============================================================
    // CONEXIONES — EXPANDIR / COLAPSAR TODOS
    // ============================================================

    connect(btnExpand, &QCheckBox::clicked, this, [this](bool checked) {       
        emit expandCollapseAllRequested(checked);
    });

    // ============================================================
    // CONEXIONES — VISIBILITY (mostrar / ocultar TODOS)
    // ============================================================

    connect(btnVisibility, &QCheckBox::clicked, this, [this](bool checked) {
        emit visibilityAllRequested(checked);
    });
    
    // ============================================================
    // CONEXIONES — AÑADIR FILTRO (botón "+")
    // ============================================================

    connect(btnAdd, &QPushButton::clicked, this, [this]() {
        emit addFilterRequested();
    });

    // ============================================================
    // CONEXIONES — ELIMINAR FILTRO/S SELECCIONADO/S
    // ============================================================

    connect(btnRemove, &QPushButton::clicked, this, [this]() {
        emit removeFilterRequested();
    });

    // ============================================================
    // CONEXIONES — CAMBIO DE CATEGORÍA (Effect / Audio)
    // ============================================================

    connect(btnEffects, &QToolButton::clicked, this, [this]() {
        emit categoryChanged(FilterCategory::Effect);
        setActiveCategory(FilterCategory::Effect);
    });

    connect(btnAudio, &QToolButton::clicked, this, [this]() {
        emit categoryChanged(FilterCategory::Audio);
        setActiveCategory(FilterCategory::Audio);
    });

    // ============================================================
    // CONEXIONES - COPIAR PEGAR FILTROS
    // ============================================================
    connect(btnCopy, &QPushButton::clicked, this, [this]() {
        emit copyRequested();
    });

    connect(btnPaste, &QPushButton::clicked, this, [this]() {
        emit pasteRequested();
    });

    // ============================================================
    // CONEXIONES — SELECT ALL (seleccionar / deseleccionar TODOS)
    // ============================================================

    connect(btnSelectAll, &QCheckBox::clicked, this, [this](bool checked) {
        emit selectAllRequested(checked);
    });

    // ============================================================
    // CONEXIONES — POPOUT ALL (abrir todos los filtros)
    // ============================================================

    connect(btnPopoutAll, &QPushButton::clicked, this, [this]() {
        emit popoutAllRequested();
    });

    // ============================================================
    // ESTADO INICIAL — Effect activo
    // ============================================================
    setActiveCategory(FilterCategory::Effect);
}

void FiltersHeader::setActiveCategory(FilterCategory category)
{
    currentCategory = category;

    if (category == FilterCategory::Effect) {
        btnEffects->setStyleSheet("border: 2px solid #FF4444;");
        btnAudio->setStyleSheet("border: 2px solid #16c416;");
    } else {
        btnEffects->setStyleSheet("border: 2px solid #16c416;");
        btnAudio->setStyleSheet("border: 2px solid #FF4444;");
    }
}


void FiltersHeader::updateAudioButtonState(bool enabled)
{
    btnAudio->setEnabled(enabled);

    if (!enabled) {
        btnAudio->setStyleSheet("border: 2px solid #444;");
        return;
    }

    // Reaplicar estilos según la categoría actual
    setActiveCategory(currentCategory);
}

void FiltersHeader::disableAudioCategory()
{
    // 1) Si la categoría activa es Audio → cambiar primero a Effect
    if (currentCategory == FilterCategory::Audio) {
        setActiveCategory(FilterCategory::Effect);
        emit categoryChanged(FilterCategory::Effect);
    }

    // 2) Ahora sí, desactivar el botón Audio
    btnAudio->setEnabled(false);

    // 3) Forzar borde gris
    btnAudio->setStyleSheet("border: 2px solid #444;");
}

