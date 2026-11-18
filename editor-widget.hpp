#pragma once

#include <QWidget>
#include <QTabWidget>

class EditorModel;
class QPropertyAnimation;

class EditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorWidget(EditorModel *model, QWidget *parent = nullptr);

    QTabWidget *getTabs() const { return tabs; }

    void showSourceTab();
    void showPropertiesTab();
    void showFiltersTab();
    void showSceneFiltersTab();
    void showTransformTab();
    void showSceneNotesTab();
    void showSourceNotesTab();

signals:
    void tabChanged(int index); 

private:
    QTabWidget *tabs  = nullptr;
    EditorModel *model = nullptr;

    void animateTabSwitch(int from, int to);

    int lastTabIndex = 0;
    QWidget *switchOldPage = nullptr;
    QWidget *switchNewPage = nullptr;
    QList<QPropertyAnimation *> tabSwitchAnims;

private slots:
    void onTabChanged(int index);    
    
};

