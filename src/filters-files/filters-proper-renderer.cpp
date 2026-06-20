#include "filters-proper-renderer.hpp"
#include "src/properties/properties-view.hpp"

#include <QVBoxLayout>

extern "C" {
#include <obs.h>
#include <obs-properties.h>
}

/* Wrapper reload */
static obs_properties_t *ReloadFilterProperties(void *obj)
{
    return obs_source_properties((obs_source_t *)obj);
}

/* Wrapper update */
static void UpdateFilterProperties(void *obj, obs_data_t *old_settings, obs_data_t *new_settings)
{
    Q_UNUSED(old_settings);
    obs_source_update((obs_source_t *)obj, new_settings);
}

FiltersProperRenderer::FiltersProperRenderer(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("E33_FilterProperties");
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

    /* 🔥 AÑADIDO: permitir expansión horizontal */
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumWidth(0);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);
}

void FiltersProperRenderer::clearProperties()
{
    if (propertiesView) {
        propertiesView->deleteLater();
        propertiesView = nullptr;
    }

    QLayoutItem *item;
    while ((item = mainLayout->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }
}

void FiltersProperRenderer::setFilter(obs_source_t *filter)
{
    currentFilter = filter;

    clearProperties();

    if (!filter)
        return;

    obs_data_t *settings = obs_source_get_settings(filter);

    propertiesView = new OBSPropertiesView(
        OBSData(settings),
        filter,
        ReloadFilterProperties,
        UpdateFilterProperties,
        nullptr,
        0
    );

    obs_data_release(settings);

    propertiesView->setScrolling(false);

    /* 🔥 AÑADIDO: permitir expansión horizontal */
    propertiesView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    propertiesView->setMinimumWidth(0);

    mainLayout->addWidget(propertiesView);
}

void FiltersProperRenderer::updateValues()
{
    if (!currentFilter || !propertiesView)
        return;

    propertiesView->ReloadProperties();
}
