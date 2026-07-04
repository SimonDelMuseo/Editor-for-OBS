#pragma once

#include <vector>
#include <string>
#include <QString>

extern "C" {
#include <obs.h>
}

struct CopiedFilterData {
    obs_data_t *settings = nullptr;
    std::string type;
    QString name;
};

void copyFilterFromSource(obs_source_t *source, size_t index);
void pasteFiltersToSource(obs_source_t *targetSource, size_t insertIndex);

void clearCopiedFilters();
bool hasCopiedFilters();
