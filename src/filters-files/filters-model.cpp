#include "filters-model.hpp"
#include "filters-copy-paste.hpp"
#include "editor-model.hpp"
#include "src/properties/properties-view.hpp"

#include <obs-module.h>

extern "C" {
#include <obs.h>
#include <obs-frontend-api.h>
}

FiltersModel::FiltersModel(QObject *parent)
    : QObject(parent)
{
}

FiltersModel::~FiltersModel()
{
    releaseFilterSignals();
}

void FiltersModel::setSource(obs_source_t *source)
{
    releaseFilterSignals();
    currentSource = source;
    connectFilterSignals();
    emit filtersChanged();
}

std::vector<obs_source_t*> FiltersModel::getFilters() const
{
    std::vector<obs_source_t*> out;

    if (!currentSource)
        return out;

    obs_source_enum_filters(
        currentSource,
        [](obs_source_t *, obs_source_t *child, void *param) {
            auto *vec = static_cast<std::vector<obs_source_t*>*>(param);
            vec->push_back(child);
        },
        &out);

    return out;
}

void FiltersModel::addFilter(const std::string &type)
{
    if (!currentSource)
        return;

    obs_data_t *settings = obs_data_create();

    const char *visible = obs_source_get_display_name(type.c_str());
    std::string name = visible ? visible : type;

    obs_source_t *filter = obs_source_create(type.c_str(), name.c_str(), settings, nullptr);

    obs_data_release(settings);

    if (!filter)
        return;

    obs_source_filter_add(currentSource, filter);
    obs_source_release(filter);

    emit filtersChanged();
}

void FiltersModel::removeFilter(size_t index)
{
    if (!currentSource)
        return;

    auto filters = getFilters();
    if (index >= filters.size())
        return;

    obs_source_t *filter = filters[index];

    obs_source_filter_remove(currentSource, filter);

    emit filtersChanged();
}

void FiltersModel::moveFilterUp(size_t index)
{
    if (!currentSource)
        return;

    auto filters = getFilters();
    if (index == 0 || index >= filters.size())
        return;

    obs_source_t *filter = filters[index];
    obs_source_filter_set_order(currentSource, filter, OBS_ORDER_MOVE_UP);

    emit filtersChanged();
}

void FiltersModel::moveFilterDown(size_t index)
{
    if (!currentSource)
        return;

    auto filters = getFilters();
    if (index >= filters.size() - 1)
        return;

    obs_source_t *filter = filters[index];
    obs_source_filter_set_order(currentSource, filter, OBS_ORDER_MOVE_DOWN);

    emit filtersChanged();
}

void FiltersModel::setFilterVisibility(size_t index, bool visible)
{
    if (!currentSource)
        return;

    auto filters = getFilters();
    if (index >= filters.size())
        return;

    obs_source_t *filter = filters[index];

    if (obs_source_enabled(filter) != visible)
        obs_source_set_enabled(filter, visible);

    emit filterVisibilityChanged(filter, visible);
}

void FiltersModel::renameFilter(size_t index, const std::string &newName)
{
    if (!currentSource || newName.empty())
        return;

    auto filters = getFilters();
    if (index >= filters.size())
        return;

    obs_source_t *filter = filters[index];

    const char *cur = obs_source_get_name(filter);
    const QString oldName = cur ? QString::fromUtf8(cur) : QString();
    if (oldName == QString::fromUtf8(newName.c_str()))
        return;

    obs_source_set_name(filter, newName.c_str());
    emit filterRenamed(filter, oldName);
}

void FiltersModel::connectFilterSignals()
{
    if (!currentSource)
        return;

    for (obs_source_t *f : getFilters()) {
        signal_handler_t *sh = obs_source_get_signal_handler(f);
        if (!sh)
            continue;
        signal_handler_connect(sh, "rename", filter_rename_cb, this);
        signal_handler_connect(sh, "enable", filter_enable_cb, this);
        connectedFilters.push_back(obs_source_get_ref(f));
    }
}

void FiltersModel::releaseFilterSignals()
{
    for (obs_source_t *f : connectedFilters) {
        if (signal_handler_t *sh = obs_source_get_signal_handler(f)) {
            signal_handler_disconnect(sh, "rename",
                                      filter_rename_cb, this);
            signal_handler_disconnect(sh, "enable",
                                      filter_enable_cb, this);
        }
        obs_source_release(f);
    }
    connectedFilters.clear();
}

void FiltersModel::filter_rename_cb(void *data, calldata_t *cd)
{
    auto *self = static_cast<FiltersModel *>(data);
    auto *src = static_cast<obs_source_t *>(calldata_ptr(cd, "source"));
    const char *prev = calldata_string(cd, "prev_name");
    const QString prevQ = prev ? QString::fromUtf8(prev) : QString();
    obs_source_t *ref = src ? obs_source_get_ref(src) : nullptr;

    QMetaObject::invokeMethod(
        self,
        [self, ref, prevQ]() {
            if (ref && self->indexOfFilter(ref) >= 0)
                emit self->filterRenamed(ref, prevQ);
            if (ref)
                obs_source_release(ref);
        },
        Qt::QueuedConnection);
}

void FiltersModel::filter_enable_cb(void *data, calldata_t *cd)
{
    auto *self = static_cast<FiltersModel *>(data);
    auto *src = static_cast<obs_source_t *>(calldata_ptr(cd, "source"));
    const bool enabled = calldata_bool(cd, "enabled");
    obs_source_t *ref = src ? obs_source_get_ref(src) : nullptr;

    QMetaObject::invokeMethod(
        self,
        [self, ref, enabled]() {
            if (ref && self->indexOfFilter(ref) >= 0)
                emit self->filterVisibilityChanged(ref, enabled);
            if (ref)
                obs_source_release(ref);
        },
        Qt::QueuedConnection);
}

std::vector<QString> FiltersModel::getAvailableFilterTypes() const
{
    std::vector<QString> out;

    size_t idx = 0;
    const char *id = nullptr;

    while (obs_enum_filter_types(idx, &id)) {
        out.push_back(QString::fromUtf8(id));
        idx++;
    }

    return out;
}

void FiltersModel::resetFilter(size_t index)
{
    if (!currentSource)
        return;

    auto filters = getFilters();
    if (index >= filters.size())
        return;

    obs_source_t *filter = filters[index];

    obs_data_t *settings = obs_source_get_settings(filter);

    obs_data_clear(settings);
    obs_source_update(filter, nullptr);
    obs_data_release(settings);

    obs_source_update_properties(filter);

    for (OBSPropertiesView *view : AllPropertyViews()) {
        if (view->IsObject(filter))
            view->ReloadProperties();
    }

    emit filtersChanged();
}

int FiltersModel::indexOfFilter(obs_source_t *filter) const
{
    if (!currentSource || !filter)
        return -1;

    auto filters = getFilters();
    for (size_t i = 0; i < filters.size(); ++i) {
        if (filters[i] == filter)
            return static_cast<int>(i);
    }

    return -1;
}

void FiltersModel::copyFilter(size_t index)
{
    if (!currentSource)
        return;

    copyFilterFromSource(currentSource, index);
}

static void restore_filters_cb(const char *data)
{
    obs_data_t *d = obs_data_create_from_json(data);
    if (!d)
        return;

    const char *uuid = obs_data_get_string(d, "uuid");
    obs_source_t *src = (uuid && *uuid) ? obs_get_source_by_uuid(uuid) : nullptr;
    obs_data_array_t *arr = obs_data_get_array(d, "array");

    if (src && arr) {
        obs_source_restore_filters(src, arr);
        obs_source_release(src);
    } else if (src) {
        obs_source_release(src);
    }

    if (arr)
        obs_data_array_release(arr);

    obs_data_release(d);

    if (EditorModel *m = EditorModel::instanceForUndo())
        m->notifyFiltersChanged();
}

void FiltersModel::pasteFilter(size_t index)
{
    if (!currentSource)
        return;

    const char *uuid = obs_source_get_uuid(currentSource);

    obs_data_array_t *undoArray = obs_source_backup_filters(currentSource);
    pasteFiltersToSource(currentSource, index);
    obs_data_array_t *redoArray = obs_source_backup_filters(currentSource);

    obs_data_t *undoWrapper = obs_data_create();
    obs_data_set_string(undoWrapper, "uuid", uuid);
    obs_data_set_array(undoWrapper, "array", undoArray);

    obs_data_t *redoWrapper = obs_data_create();
    obs_data_set_string(redoWrapper, "uuid", uuid);
    obs_data_set_array(redoWrapper, "array", redoArray);

    const char *u = obs_data_get_json(undoWrapper);
    const char *r = obs_data_get_json(redoWrapper);

    obs_frontend_add_undo_redo_action(obs_module_text("UndoFiltersPaste"),
                                      restore_filters_cb, restore_filters_cb,
                                      u ? u : "{}", r ? r : "{}", false);

    obs_data_release(undoWrapper);
    obs_data_release(redoWrapper);
    obs_data_array_release(undoArray);
    obs_data_array_release(redoArray);

    emit filtersChanged();
}
