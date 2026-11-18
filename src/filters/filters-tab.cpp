#include "filters-tab.hpp"
#include "editor-model.hpp"

#include "../filters-files/filters-model.hpp"
#include "../filters-files/filters-proper-renderer.hpp"
#include "../filters-files/filters-renderer.hpp"
#include "../filters-files/filters-state.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QResizeEvent>

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

    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *scrollWidget = new QWidget(scrollArea);
    auto *formLayout = new QFormLayout(scrollWidget);
    formLayout->setContentsMargins(8, 8, 8, 8);
    formLayout->setSpacing(6);
    scrollWidget->setLayout(formLayout);

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);

    filtersModel    = new FiltersModel(this);
    filtersRenderer = new FiltersRenderer(this);
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
                this, &FiltersTab::refreshUI);

        connect(model, &EditorModel::filtersChangedExternally, this,
                [this]() {
                    if (filtersModel && lastSource)
                        filtersModel->setSource(lastSource);
                });
    }

    refreshUI();
}

void FiltersTab::refreshUI()
{
    if (!model || !filtersModel)
        return;

    obs_source_t *src = model->selectedSource();

    if (src != lastSource) {
        lastSource = src;
        filtersModel->setSource(src);

        properRenderer->setFilter(nullptr);
        return;
    }

    if (src) {
        properRenderer->updateValues();
    }
}

void FiltersTab::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    auto *sa = findChild<QScrollArea *>();
    if (!sa)
        return;

    QWidget *sw = sa->widget();
    blog(LOG_INFO,
         "[E33][ancho] tabW=%d vpW=%d | scrollWidget minW=%d hintW=%d | "
         "list minW=%d hintW=%d | hbarVisible=%d hbarMax=%d | tab minW=%d",
         width(), sa->viewport()->width(),
         sw ? sw->minimumSizeHint().width() : -1,
         sw ? sw->sizeHint().width() : -1,
         filtersRenderer ? filtersRenderer->minimumSizeHint().width() : -1,
         filtersRenderer ? filtersRenderer->sizeHint().width() : -1,
         (int)sa->horizontalScrollBar()->isVisible(),
         sa->horizontalScrollBar()->maximum(),
         minimumWidth());
}
