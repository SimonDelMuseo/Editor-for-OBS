#pragma once

#include <QObject>
#include <unordered_set>
#include <QString>

class FiltersState : public QObject {
    Q_OBJECT

public:
    explicit FiltersState(QObject *parent = nullptr);

    bool isExpanded(const QString &name) const;
    void setExpanded(const QString &name, bool expanded);

    void expandAll(const std::vector<QString> &names);
    void collapseAll();

private:
    std::unordered_set<QString> expandedFilters;
};
