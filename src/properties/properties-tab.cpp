#include "properties-tab.hpp"
#include "editor-model.hpp"

extern "C" {
#include <obs.h>
#include <obs-properties.h>
}

#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>

#include "properties-view.hpp"

static obs_properties_t *ReloadSourceProperties(void *obj)
{
    return obs_source_properties((obs_source_t *)obj);
}

static void UpdateSourceProperties(void *obj, obs_data_t *old_settings, obs_data_t *new_settings)
{
    Q_UNUSED(old_settings);
    obs_source_update((obs_source_t *)obj, new_settings);
}

PropertiesTab::PropertiesTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    connect(model, &EditorModel::modelUpdated,
            this, &PropertiesTab::refreshUI);

    refreshUI();
}

void PropertiesTab::showNoSourceMessage()
{
    if (propertiesUI) {
        propertiesUI->deleteLater();
        propertiesUI = nullptr;
    }

    // limpiar cualquier widget previo del layout
    QLayoutItem *item;
    while ((item = mainLayout->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    QLabel *label = new QLabel("No source selected");
    mainLayout->addWidget(label);
}

void PropertiesTab::refreshUI()
{
    obs_source_t *source = model->selectedSource();

    if (!source) {
        lastSource = nullptr;
        showNoSourceMessage();
        return;
    }

    if (source != lastSource) {
        lastSource = source;
        buildPropertiesUI(source);
    }
}

void PropertiesTab::buildPropertiesUI(obs_source_t *source)
{
    if (propertiesUI) {
        propertiesUI->deleteLater();
        propertiesUI = nullptr;
    }

    // limpiar cualquier widget previo del layout
    QLayoutItem *item;
    while ((item = mainLayout->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    if (!source) {
        showNoSourceMessage();
        return;
    }

    obs_data_t *settings = obs_source_get_settings(source);

    propertiesUI = new OBSPropertiesView(
        OBSData(settings),
        source,
        ReloadSourceProperties,
        UpdateSourceProperties,
        nullptr,
        0
    );

    obs_data_release(settings);

    mainLayout->addWidget(propertiesUI);
}
