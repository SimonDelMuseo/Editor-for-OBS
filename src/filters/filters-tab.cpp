#include "filters-tab.hpp"
#include "editor-model.hpp"

#include "../filters-files/filters-model.hpp"
#include "../filters-files/filters-proper-renderer.hpp"
#include "../filters-files/filters-renderer.hpp"
#include "../filters-files/filters-state.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QScrollArea>

extern "C" {
#include <obs.h>
}

FiltersTab::FiltersTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    /* 🔥 AÑADIDO: desactivar scroll horizontal */
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *scrollWidget = new QWidget(scrollArea);
    auto *formLayout = new QFormLayout(scrollWidget);
    formLayout->setContentsMargins(8, 8, 8, 8);
    formLayout->setSpacing(6);
    scrollWidget->setLayout(formLayout);

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);

    /* MODELO + RENDERERS */
    filtersModel    = new FiltersModel(this);
    filtersRenderer = new FiltersRenderer(this);
    properRenderer  = new FiltersProperRenderer(this);

    filtersRenderer->setModel(filtersModel);

    /* CONEXIONES DE MOVER ARRIBA/ABAJO */
    connect(filtersRenderer, &FiltersRenderer::moveFilterUpRequested,
            filtersModel,    &FiltersModel::moveFilterUp);

    connect(filtersRenderer, &FiltersRenderer::moveFilterDownRequested,
            filtersModel,    &FiltersModel::moveFilterDown);

    /* 🔥 NUEVO: CONEXIONES COPIAR / PEGAR */
    connect(filtersRenderer, &FiltersRenderer::copyRequested,
            filtersModel,    [this](int index) {
                filtersModel->copyFilter(index);
            });

    connect(filtersRenderer, &FiltersRenderer::pasteRequested,
            filtersModel,    [this](int index) {
                filtersModel->pasteFilter(index);
            });

    /* LISTA DE FILTROS */
    formLayout->addRow(filtersRenderer);

    /* PROPIEDADES DEL FILTRO SELECCIONADO */
    formLayout->addRow(properRenderer);

    if (model) {
        connect(model, &EditorModel::modelUpdated,
                this, &FiltersTab::refreshUI);
    }

    refreshUI();
}

void FiltersTab::refreshUI()
{
    if (!model || !filtersModel)
        return;

    obs_source_t *src = model->selectedSource();

    // ----------------------------------------------------
    // SI LA FUENTE HA CAMBIADO → RECONSTRUIR COMPLETAMENTE
    // ----------------------------------------------------
    if (src != lastSource) {
        lastSource = src;
        filtersModel->setSource(src);

        // Al cambiar de fuente, limpiamos propiedades
        properRenderer->setFilter(nullptr);
        return;
    }

    // ----------------------------------------------------
    // MISMA FUENTE → SOLO ACTUALIZAR PROPIEDADES
    // ----------------------------------------------------
    if (src) {
        properRenderer->updateValues();
    }
}
