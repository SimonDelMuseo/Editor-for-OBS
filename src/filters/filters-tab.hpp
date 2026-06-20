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

class FiltersTab : public QWidget
{
    Q_OBJECT

public:
    explicit FiltersTab(EditorModel *model, QWidget *parent = nullptr);

private slots:
    void refreshUI();

private:
    EditorModel *model = nullptr;

    FiltersModel          *filtersModel    = nullptr;
    FiltersRenderer       *filtersRenderer = nullptr;
    FiltersProperRenderer *properRenderer  = nullptr;

    obs_source_t *lastSource = nullptr;
};
