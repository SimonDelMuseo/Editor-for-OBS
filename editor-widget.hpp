#pragma once

#include <QWidget>
#include <QTabWidget>

class EditorModel;

class EditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorWidget(EditorModel *model, QWidget *parent = nullptr);

    QTabWidget *getTabs() const { return tabs; }

    /* ────────────────────────────────────────────────
       NUEVO — MÉTODO PÚBLICO PARA CAMBIAR A LA TAB
       DE FILTROS DE ESCENA (índice 3)
       ──────────────────────────────────────────────── */
    void showSourceTab();
    void showPropertiesTab();
    void showFiltersTab();
    void showSceneFiltersTab();
    void showTransformTab();
    void showMiscTab();

signals:
    void tabChanged(int index); 

private:
    QTabWidget *tabs  = nullptr;
    EditorModel *model = nullptr;

private slots:
    void onTabChanged(int index);    
    
};

