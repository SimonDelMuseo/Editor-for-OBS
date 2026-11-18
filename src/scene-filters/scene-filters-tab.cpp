#include "scene-filters-tab.hpp"
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

SceneFiltersTab::SceneFiltersTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    QWidget *scrollWidget = new QWidget(scrollArea);
    auto *formLayout = new QFormLayout(scrollWidget);
    formLayout->setContentsMargins(8, 8, 8, 8);
    formLayout->setSpacing(6);
    scrollWidget->setLayout(formLayout);

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);

    filtersModel    = new FiltersModel(this);
    filtersRenderer = new FiltersRenderer(this);
    filtersState    = new FiltersState(this);
    properRenderer  = new FiltersProperRenderer(this);

    filtersRenderer->setModel(filtersModel);

    connect(filtersRenderer, &FiltersRenderer::moveFilterUpRequested,
            filtersModel,    &FiltersModel::moveFilterUp);

    connect(filtersRenderer, &FiltersRenderer::moveFilterDownRequested,
            filtersModel,    &FiltersModel::moveFilterDown);

    connect(filtersRenderer, &FiltersRenderer::copyRequested,
            filtersModel,    [this](int index) {
                filtersModel->copyFilter(index);
            });

    connect(filtersRenderer, &FiltersRenderer::pasteRequested,
            filtersModel,    [this](int index) {
                filtersModel->pasteFilter(index);
            });
            
    formLayout->addRow(filtersRenderer);

    formLayout->addRow(properRenderer);

    if (model) {
        connect(model, &EditorModel::modelUpdated,
                this, &SceneFiltersTab::refreshUI);

        connect(model, &EditorModel::filtersChangedExternally, this,
                [this]() {
                    if (filtersModel && lastSource)
                        filtersModel->setSource(lastSource);
                });
    }

    refreshUI();
}

void SceneFiltersTab::refreshUI()
{
    if (!model || !filtersModel)
        return;

    obs_source_t *sceneSource = model->currentSceneSource();

    if (sceneSource != lastSource) {
        lastSource = sceneSource;
        filtersModel->setSource(sceneSource);
        properRenderer->setFilter(nullptr);
        return;
    }

    if (sceneSource) {
        properRenderer->updateValues();
    }
}
