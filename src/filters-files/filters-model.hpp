#pragma once

#include <QObject>
#include <vector>
#include <string>

extern "C" {
#include <obs.h>
}

class FiltersModel : public QObject {
    Q_OBJECT

public:
    explicit FiltersModel(QObject *parent = nullptr);
    ~FiltersModel();

    void setSource(obs_source_t *source);

    std::vector<obs_source_t*> getFilters() const;
    void addFilter(const std::string &type);
    void removeFilter(size_t index);
    void moveFilterUp(size_t index);
    void moveFilterDown(size_t index);
    void setFilterVisibility(size_t index, bool visible);
    void renameFilter(size_t index, const std::string &newName);

    // -----------------------------------------
    // Tipos de filtros disponibles
    // -----------------------------------------
    std::vector<QString> getAvailableFilterTypes() const;

    // -----------------------------------------
    // Resetear propiedades del filtro
    // -----------------------------------------
    void resetFilter(size_t index);

    // -----------------------------------------
    // Obtener índice REAL del filtro
    // -----------------------------------------
    int indexOfFilter(obs_source_t *filter) const;

    // -----------------------------------------
    // Copiar / pegar filtros (multi-copia)
    // -----------------------------------------
    void copyFilter(size_t index);
    void pasteFilter(size_t index);

    // -----------------------------------------
    // NUEVO: limpiar el buffer de copia
    // (llamado desde FiltersRenderer antes de copiar)
    // -----------------------------------------
    void clearCopiedFilters();

signals:
    void filtersChanged();

private:
    obs_source_t *currentSource = nullptr;

    // -----------------------------------------
    // NUEVO: estructura para soportar copia múltiple
    // -----------------------------------------
    struct CopiedFilter {
        obs_data_t *settings = nullptr;
        std::string type;
        QString name;
    };

    // -----------------------------------------
    // NUEVO: buffer múltiple
    // -----------------------------------------
    std::vector<CopiedFilter> copiedFilters;
};
