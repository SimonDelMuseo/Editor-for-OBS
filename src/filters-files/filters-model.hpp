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
    obs_source_t *getSource() const { return currentSource; }

    std::vector<obs_source_t*> getFilters() const;
    void addFilter(const std::string &type);
    void removeFilter(size_t index);
    void moveFilterUp(size_t index);
    void moveFilterDown(size_t index);
    void setFilterVisibility(size_t index, bool visible);
    void renameFilter(size_t index, const std::string &newName);

    std::vector<QString> getAvailableFilterTypes() const;
    void resetFilter(size_t index);
    int indexOfFilter(obs_source_t *filter) const;

    void copyFilter(size_t index);
    void pasteFilter(size_t index);

signals:
    void filtersChanged();

    void filterVisibilityChanged(obs_source_t *filter, bool enabled);
    void filterRenamed(obs_source_t *filter, const QString &oldName);

private:
    obs_source_t *currentSource = nullptr;

    std::vector<obs_source_t *> connectedFilters;
    void connectFilterSignals();
    void releaseFilterSignals();
    static void filter_rename_cb(void *data, calldata_t *cd);
    static void filter_enable_cb(void *data, calldata_t *cd);
};
