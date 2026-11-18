#include "filters-proper-renderer.hpp"
#include "src/properties/properties-view.hpp"
#include "editor-model.hpp"

#include <QVBoxLayout>
#include <QMainWindow>
#include <QApplication>

#include <cstring>

extern "C" {
#include <obs.h>
#include <obs-properties.h>
#include <obs-frontend-api.h>
}
#include <obs-module.h>

static obs_properties_t *ReloadFilterProperties(void *obj)
{
    return obs_source_properties((obs_source_t *)obj);
}

static void refreshPropertiesViewsFor(obs_source_t *target, bool skipFocused = false)
{
    if (!target)
        return;

    QWidget *focus = QApplication::focusWidget();

    for (OBSPropertiesView *view : AllPropertyViews()) {
        if (!view->IsObject(target))
            continue;
        if (skipFocused && focus && view->isAncestorOf(focus))
            continue;
        view->ReloadProperties();
    }
}

static void filter_props_undo_cb(const char *data)
{
    blog(LOG_INFO, "[E33][undo] POP-FILTRO data=%.120s", data ? data : "(null)");
    obs_data_t *dat = obs_data_create_from_json(data);
    if (!dat)
        return;

    const char *uuid = obs_data_get_string(dat, "uuid");
    obs_source_t *filter = (uuid && *uuid) ? obs_get_source_by_uuid(uuid) : nullptr;
    obs_data_t *new_settings =
        obs_data_create_from_json(obs_data_get_string(dat, "settings"));

    if (filter && new_settings) {
        obs_data_t *current = obs_source_get_settings(filter);
        obs_data_clear(current);
        obs_data_release(current);

        obs_source_update(filter, new_settings);
        obs_source_update_properties(filter);
        refreshPropertiesViewsFor(filter);

        obs_source_release(filter);
    } else if (filter) {
        obs_source_release(filter);
    }

    if (new_settings)
        obs_data_release(new_settings);
    obs_data_release(dat);

    if (EditorModel *m = EditorModel::instanceForUndo()) {
        m->modelUpdated();
        m->notifyFiltersChanged();
    }
}

static void UpdateFilterProperties(void *obj, obs_data_t *old_settings, obs_data_t *new_settings)
{
    obs_source_t *filter = (obs_source_t *)obj;
    if (!filter)
        return;

    obs_data_t *undo_wrapper = obs_data_create();
    obs_data_set_string(undo_wrapper, "uuid", obs_source_get_uuid(filter));
    obs_data_set_string(undo_wrapper, "settings", obs_data_get_json(old_settings));

    obs_data_t *redo_wrapper = obs_data_create();
    obs_data_set_string(redo_wrapper, "uuid", obs_source_get_uuid(filter));
    obs_data_set_string(redo_wrapper, "settings", obs_data_get_json(new_settings));

    const char *u = obs_data_get_json(undo_wrapper);
    const char *r = obs_data_get_json(redo_wrapper);

    if (u && r && strcmp(u, r) != 0) {
        blog(LOG_INFO, "[E33][undo] REG-FILTRO '%s' old=%s",
             obs_source_get_name(filter),
             old_settings ? "pre-cambio(BUENA)" : "NULO(basura)");

        QString label = QString(obs_module_text("UndoFilterProperties"))
                            .arg(QString::fromUtf8(obs_source_get_name(filter)));

        obs_frontend_add_undo_redo_action(label.toUtf8().constData(),
                                          filter_props_undo_cb, filter_props_undo_cb,
                                          u, r, false);
    }

    obs_data_release(undo_wrapper);
    obs_data_release(redo_wrapper);

    obs_source_update(filter, new_settings);

    refreshPropertiesViewsFor(filter, true);
}

FiltersProperRenderer::FiltersProperRenderer(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("E33_FilterProperties");
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumWidth(0);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    hide();
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

    if (!filter) {
        hide();
        return;
    }
    show();

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

    propertiesView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    propertiesView->setMinimumWidth(0);

    propertiesView->setScrolling(false);

    mainLayout->addWidget(propertiesView);
}

void FiltersProperRenderer::updateValues()
{
    if (!currentFilter || !propertiesView)
        return;

    propertiesView->ReloadProperties();
}
