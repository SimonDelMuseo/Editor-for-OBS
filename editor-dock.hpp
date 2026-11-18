#pragma once

#include <QFrame>
#include <QList>
#include <QPointer>

class EditorWidget;
class EditorModel;
class Information;
class FiltersHeader;
class FiltersRenderer;
class Header;
class QPropertyAnimation;
class QResizeEvent;

struct obs_source;

class EditorDock : public QFrame
{
    Q_OBJECT

public:
    explicit EditorDock(EditorModel *model, QWidget *parent = nullptr);
    ~EditorDock();

    void registerPopout(QWidget *w);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    EditorWidget *editorWidget = nullptr;
    EditorModel  *model        = nullptr;
    Information *information   = nullptr;

    QWidget *topBar     = nullptr;
    QWidget *bottomBar  = nullptr;

    FiltersHeader *filtersHeader = nullptr;
    Header *header = nullptr;

    QWidget *filtersBar = nullptr;
    void setFiltersBarVisible(bool show);
    QPropertyAnimation *filtersBarAnim = nullptr;
    bool filtersBarShown = false;

    QList<QPointer<QWidget>> openPopouts;

    void refreshUI();

    FiltersRenderer *currentRenderer() const;

    void refreshFiltersHeaderStates();

    void applyDefaultCategoryForSource();
    obs_source *lastCategorySource = nullptr;

    void registerHotkeys();
    void unregisterHotkeys();
    void hotkeyDispatch(size_t id);
    size_t hotkeyIds[8] = {};

private slots:
    void onTabChanged(int index);
};
