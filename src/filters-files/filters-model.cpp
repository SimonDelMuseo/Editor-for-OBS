#include "filters-model.hpp"

extern "C" {
#include <obs.h>
}

/* ────────────────────────────────────────────────
   CONSTRUCTOR
   ──────────────────────────────────────────────── */
FiltersModel::FiltersModel(QObject *parent)
    : QObject(parent)
{
}

/* ────────────────────────────────────────────────
   DESTRUCTOR — liberar copias múltiples
   ──────────────────────────────────────────────── */
FiltersModel::~FiltersModel()
{
    clearCopiedFilters();
}

/* ────────────────────────────────────────────────
   SET SOURCE
   ──────────────────────────────────────────────── */
void FiltersModel::setSource(obs_source_t *source)
{
    currentSource = source;
    emit filtersChanged();
}

/* ────────────────────────────────────────────────
   ENUMERAR FILTROS
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   AÑADIR FILTRO
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   ELIMINAR FILTRO
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   MOVER FILTRO ARRIBA
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   MOVER FILTRO ABAJO
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   VISIBILIDAD DEL FILTRO
   ──────────────────────────────────────────────── */
void FiltersModel::setFilterVisibility(size_t, bool)
{
}

/* ────────────────────────────────────────────────
   RENOMBRAR FILTRO
   ──────────────────────────────────────────────── */
void FiltersModel::renameFilter(size_t, const std::string &)
{
}

/* ────────────────────────────────────────────────
   TIPOS DE FILTRO DISPONIBLES
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   RESET FILTER — RESTAURAR AJUSTES POR DEFECTO
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   OBTENER ÍNDICE REAL DEL FILTRO
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   LIMPIAR BUFFER DE COPIA (MULTI-COPIA)
   ──────────────────────────────────────────────── */
void FiltersModel::clearCopiedFilters()
{
    for (auto &cf : copiedFilters) {
        if (cf.settings) {
            obs_data_release(cf.settings);
            cf.settings = nullptr;
        }
    }
    copiedFilters.clear();
}

/* ────────────────────────────────────────────────
   COPIAR FILTRO (AÑADE AL VECTOR)
   ──────────────────────────────────────────────── */
void FiltersModel::copyFilter(size_t index)
{
    if (!currentSource)
        return;

    auto filters = getFilters();
    if (index >= filters.size())
        return;

    obs_source_t *filter = filters[index];

    CopiedFilter cf;
    cf.type = obs_source_get_id(filter);
    cf.name = QString::fromUtf8(obs_source_get_name(filter));

    obs_data_t *settings = obs_source_get_settings(filter);
    if (settings) {
        cf.settings = obs_data_create();
        obs_data_apply(cf.settings, settings);
        obs_data_release(settings);
    }

    copiedFilters.push_back(cf);
}

/* ────────────────────────────────────────────────
   PEGAR FILTROS (TODOS LOS COPIADOS)
   ──────────────────────────────────────────────── */
void FiltersModel::pasteFilter(size_t index)
{
    if (!currentSource || copiedFilters.empty())
        return;

    // Obtener lista actual SOLO para generar nombres únicos
    auto filters = getFilters();

    // Si el índice no es válido en la nueva fuente → pegar al final
    if (index > filters.size())
        index = filters.size();

    size_t insertPos = index;

    for (const auto &cf : copiedFilters) {

        // -------------------------------
        // Generar nombre único
        // -------------------------------
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

        // -------------------------------
        // Crear el nuevo filtro
        // -------------------------------
        obs_source_t *newFilter =
            obs_source_create(cf.type.c_str(),
                              newName.toUtf8().constData(),
                              cf.settings,
                              nullptr);

        if (!newFilter)
            continue;

        // Añadir al final de la nueva fuente
        obs_source_filter_add(currentSource, newFilter);

        // Recalcular lista actualizada
        filters = getFilters();

        // Encontrar índice real del nuevo filtro
        int newIndex = -1;
        for (size_t i = 0; i < filters.size(); ++i) {
            if (filters[i] == newFilter) {
                newIndex = (int)i;
                break;
            }
        }

        // -------------------------------
        // Moverlo hasta insertPos
        // -------------------------------
        if (newIndex >= 0 && insertPos < (size_t)newIndex) {
            while (newIndex > (int)insertPos) {
                obs_source_filter_set_order(currentSource, newFilter, OBS_ORDER_MOVE_UP);
                newIndex--;
            }
        }

        obs_source_release(newFilter);

        // Avanzar posición para el siguiente filtro copiado
        insertPos++;
    }

    emit filtersChanged();
}
