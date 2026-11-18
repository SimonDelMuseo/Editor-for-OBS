#include "filters-state.hpp"

FiltersState::FiltersState(QObject *parent)
    : QObject(parent)
{
}

bool FiltersState::isExpanded(const QString &name) const
{
    return expandedFilters.count(name) > 0;
}

void FiltersState::setExpanded(const QString &name, bool expanded)
{
    if (expanded)
        expandedFilters.insert(name);
    else
        expandedFilters.erase(name);
}

void FiltersState::rename(const QString &oldName, const QString &newName)
{
    auto it = expandedFilters.find(oldName);
    if (it != expandedFilters.end()) {
        expandedFilters.erase(it);
        expandedFilters.insert(newName);
    }
}

void FiltersState::expandAll(const std::vector<QString> &names)
{
    expandedFilters.clear();
    for (const auto &n : names)
        expandedFilters.insert(n);
}

void FiltersState::collapseAll()
{
    expandedFilters.clear();
}
