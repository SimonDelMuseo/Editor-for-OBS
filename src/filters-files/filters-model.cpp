#include "filters-model.hpp"
#include "filters-copy-paste.hpp"

extern "C" {
#include <obs.h>
}

FiltersModel::FiltersModel(QObject *parent)
    : QObject(parent)
{
}

FiltersModel::~FiltersModel()
{
}

void FiltersModel::setSource(obs_source_t *source)
{
    currentSource = source;
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

void FiltersModel::setFilterVisibility(size_t, bool)
{
}

void FiltersModel::renameFilter(size_t, const std::string &)
{
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

void FiltersModel::pasteFilter(size_t index)
{
    if (!currentSource)
        return;

    pasteFiltersToSource(currentSource, index);
    emit filtersChanged();
}
