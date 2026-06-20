#pragma once
#include <QWidget>

extern "C" {
#include <obs.h>
}

class EditorModel;
class FiltersModel;
class FiltersRenderer;
class FiltersState;
class FiltersProperRenderer;

class SceneFiltersTab : public QWidget
{
    Q_OBJECT

public:
    explicit SceneFiltersTab(EditorModel *model, QWidget *parent = nullptr);

private slots:
    void refreshUI();

private:
    EditorModel *model = nullptr;

    FiltersModel          *filtersModel      = nullptr;
    FiltersRenderer       *filtersRenderer   = nullptr;
    FiltersState          *filtersState      = nullptr;
    FiltersProperRenderer *properRenderer    = nullptr;

    obs_source_t *lastSource = nullptr;   // ← NECESARIO
};
