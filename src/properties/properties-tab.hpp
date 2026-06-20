#pragma once

#include <QWidget>

extern "C" {
#include <obs.h>
}

class EditorModel;
class QVBoxLayout;
class OBSPropertiesView;

class PropertiesTab : public QWidget
{
    Q_OBJECT
public:
    explicit PropertiesTab(EditorModel *model, QWidget *parent = nullptr);

private slots:
    void refreshUI();

private:
    void buildPropertiesUI(obs_source_t *source);
    void showNoSourceMessage();

private:
    EditorModel        *model        = nullptr;
    QVBoxLayout        *mainLayout   = nullptr;
    OBSPropertiesView  *propertiesUI = nullptr;

    obs_source_t       *lastSource   = nullptr;
};
