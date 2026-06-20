#pragma once

#include <QFrame>

extern "C" {
#include <obs.h>
}

class QVBoxLayout;
class OBSPropertiesView;

class FiltersProperRenderer : public QFrame {
    Q_OBJECT

public:
    explicit FiltersProperRenderer(QWidget *parent = nullptr);

    void setFilter(obs_source_t *filter);
    void updateValues();

private:
    void clearProperties();

private:
    QVBoxLayout       *mainLayout     = nullptr;
    OBSPropertiesView *propertiesView = nullptr;

    obs_source_t *currentFilter = nullptr;
};
