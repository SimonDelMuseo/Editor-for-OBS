#include "filters-copy-paste.hpp"

static std::vector<CopiedFilterData> gCopiedFilters;

/* ---------------------------------------------------------
   COPIAR FILTRO DESDE UN SOURCE
   --------------------------------------------------------- */
void copyFilterFromSource(obs_source_t *source, size_t index)
{
    if (!source)
        return;

    std::vector<obs_source_t*> filters;

    obs_source_enum_filters(
        source,
        [](obs_source_t *, obs_source_t *child, void *param) {
            auto *vec = static_cast<std::vector<obs_source_t*>*>(param);
            vec->push_back(child);
        },
        &filters);

    if (index >= filters.size())
        return;

    obs_source_t *filter = filters[index];

    CopiedFilterData cf;
    cf.type = obs_source_get_id(filter);
    cf.name = QString::fromUtf8(obs_source_get_name(filter));

    obs_data_t *settings = obs_source_get_settings(filter);
    if (settings) {
        cf.settings = obs_data_create();
        obs_data_apply(cf.settings, settings);
        obs_data_release(settings);
    }

    gCopiedFilters.push_back(cf);
}

/* ---------------------------------------------------------
   PEGAR FILTROS COPIADOS EN UN SOURCE DESTINO
   --------------------------------------------------------- */
void pasteFiltersToSource(obs_source_t *targetSource, size_t insertIndex)
{
    if (!targetSource || gCopiedFilters.empty())
        return;

    std::vector<obs_source_t*> filters;

    obs_source_enum_filters(
        targetSource,
        [](obs_source_t *, obs_source_t *child, void *param) {
            auto *vec = static_cast<std::vector<obs_source_t*>*>(param);
            vec->push_back(child);
        },
        &filters);

    if (insertIndex > filters.size())
        insertIndex = filters.size();

    size_t insertPos = insertIndex;

    for (const auto &cf : gCopiedFilters) {

        QString baseName = cf.name;
        QString newName = baseName;
        int suffix = 2;

        bool exists = true;
        while (exists) {
            exists = false;
            for (auto *ff : filters) {
                if (QString::fromUtf8(obs_source_get_name(ff)) == newName) {
                    newName = baseName + " " + QString::number(suffix++);
                    exists = true;
                    break;
                }
            }
        }

        obs_source_t *newFilter =
            obs_source_create(cf.type.c_str(),
                              newName.toUtf8().constData(),
                              cf.settings,
                              nullptr);

        if (!newFilter)
            continue;

        obs_source_filter_add(targetSource, newFilter);

        filters.clear();
        obs_source_enum_filters(
            targetSource,
            [](obs_source_t *, obs_source_t *child, void *param) {
                auto *vec = static_cast<std::vector<obs_source_t*>*>(param);
                vec->push_back(child);
            },
            &filters);

        int newIndex = -1;
        for (size_t i = 0; i < filters.size(); ++i) {
            if (filters[i] == newFilter) {
                newIndex = (int)i;
                break;
            }
        }

        if (newIndex >= 0 && insertPos < (size_t)newIndex) {
            while (newIndex > (int)insertPos) {
                obs_source_filter_set_order(targetSource, newFilter, OBS_ORDER_MOVE_UP);
                newIndex--;
            }
        }

        obs_source_release(newFilter);

        insertPos++;
    }
}

/* ---------------------------------------------------------
   LIMPIAR BUFFER GLOBAL
   --------------------------------------------------------- */
void clearCopiedFilters()
{
    for (auto &cf : gCopiedFilters) {
        if (cf.settings) {
            obs_data_release(cf.settings);
            cf.settings = nullptr;
        }
    }
    gCopiedFilters.clear();
}

bool hasCopiedFilters()
{
    return !gCopiedFilters.empty();
}
