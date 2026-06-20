/* ────────────────────────────────────────────────
   INCLUDES
   ──────────────────────────────────────────────── */
#include <obs-module.h>

#include "editor-widget.hpp"
#include "editor-model.hpp"

#include "src/source/source-tab.hpp"
#include "src/properties/properties-tab.hpp"
#include "src/filters/filters-tab.hpp"
#include "src/scene-filters/scene-filters-tab.hpp"
#include "src/transform/transform-tab.hpp"
#include "src/misc/misc-tab.hpp"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QTabBar>

/* ────────────────────────────────────────────────
   CONSTRUCTOR — WIDGET INTERNO EXACTO AL MODELO EXELDRO
   ──────────────────────────────────────────────── */
EditorWidget::EditorWidget(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    tabs = new QTabWidget(this);

    tabs->addTab(new SourceTab(model, this),       obs_module_text("SourceTab"));
    tabs->addTab(new PropertiesTab(model, this),   obs_module_text("PropertiesTab"));
    tabs->addTab(new FiltersTab(model, this),      obs_module_text("FiltersTab"));
    tabs->addTab(new SceneFiltersTab(model, this), obs_module_text("SceneFiltersTab"));
    tabs->addTab(new TransformTab(model, this),    obs_module_text("TransformTab"));
    tabs->addTab(new MiscTab(model, this),         obs_module_text("MiscTab"));

    /* ============================================================
       OCULTAR LAS PESTAÑAS (solo la barra, NO el contenido)
       ============================================================ */
    tabs->tabBar()->hide();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabs);

   connect(tabs, &QTabWidget::currentChanged,
        this, &EditorWidget::onTabChanged);

}

/* ────────────────────────────────────────────────
   NUEVO — CAMBIAR A LA TAB DE FUENTE
   (índice 0 en el QTabWidget)
   ──────────────────────────────────────────────── */
void EditorWidget::showSourceTab()
{
    if (tabs)
        tabs->setCurrentIndex(0);
}

/* ────────────────────────────────────────────────
   NUEVO — CAMBIAR A LA TAB DE PROPIEDADES
   (índice 1 en el QTabWidget)
   ──────────────────────────────────────────────── */
void EditorWidget::showPropertiesTab()
{
    if (tabs)
        tabs->setCurrentIndex(1);
}

/* ────────────────────────────────────────────────
   NUEVO — CAMBIAR A LA TAB DE FILTROS DE FUENTE
   (índice 2 en el QTabWidget)
   ──────────────────────────────────────────────── */
void EditorWidget::showFiltersTab()
{
    if (tabs)
        tabs->setCurrentIndex(2);
}

/* ────────────────────────────────────────────────
   NUEVO — CAMBIAR A LA TAB DE FILTROS DE ESCENA
   (índice 3 en el QTabWidget)
   ──────────────────────────────────────────────── */
void EditorWidget::showSceneFiltersTab()
{
    if (tabs)
        tabs->setCurrentIndex(3);
}

/* ────────────────────────────────────────────────
   NUEVO — CAMBIAR A LA TAB DE TRANSFORMAR
   (índice 4 en el QTabWidget)
   ──────────────────────────────────────────────── */
void EditorWidget::showTransformTab()
{
    if (tabs)
        tabs->setCurrentIndex(4);
}

/* ────────────────────────────────────────────────
   NUEVO — CAMBIAR A LA TAB DE MISC
   (índice 5 en el QTabWidget)
   ──────────────────────────────────────────────── */
void EditorWidget::showMiscTab()
{
    if (tabs)
        tabs->setCurrentIndex(5);
}

/* ────────────────────────────────────────────────
   NUEVO — SLOT INTERNO QUE REENVÍA EL CAMBIO
   ──────────────────────────────────────────────── */
void EditorWidget::onTabChanged(int index)
{
    emit tabChanged(index);
}