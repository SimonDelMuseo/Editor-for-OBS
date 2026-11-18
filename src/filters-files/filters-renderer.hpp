#pragma once

#include <obs.h>
#include <obs-source.h>

#include <QWidget>
#include <QLineEdit>
#include <QVector>

class QVBoxLayout;
class FiltersModel;
class FiltersState;
class QPropertyAnimation;

class FilterNameEdit : public QLineEdit {
    Q_OBJECT
public:
    using QLineEdit::QLineEdit;

signals:
    void lostFocus();

protected:
    void focusOutEvent(QFocusEvent *e) override {
        emit lostFocus();
        QLineEdit::focusOutEvent(e);
    }
};

class FiltersRenderer : public QWidget {
    Q_OBJECT

public:
    explicit FiltersRenderer(QWidget *parent = nullptr);

    void setModel(FiltersModel *model);
    void refresh();

    void toggleAllExpanded(bool expanded);
    void setAllVisible(bool visible);
    void setAllSelected(bool selected);
    void popoutAll();

    void markExpanded(const QString &name);

    FiltersModel* getModel() const { return model; }

    enum class FilterCategory { Effect, Audio };
    void setCategory(FilterCategory c);

public slots:
    void removeSelectedFilters();

    void copyFromHeader();
    void pasteFromHeader();
    
signals:
    void filterExpanded(int index);
    void filterCollapsed(int index);
    void moveFilterUpRequested(int index);
    void moveFilterDownRequested(int index);
    void openExternalWindow(int index);
    void resetFilterRequested(int index);

    void popoutCreated(QWidget *w);

    void copyRequested(int index);
    void pasteRequested(int index);

    void selectionStateChanged();

private:
    FiltersModel *model = nullptr;
    QVBoxLayout *mainLayout = nullptr;

    FilterCategory currentCategory = FilterCategory::Effect;

    std::vector<obs_source_t*> effectFilters;
    std::vector<obs_source_t*> audioFilters;

    FiltersState *state = nullptr;

    void rebuildUI();
    bool eventFilter(QObject *obj, QEvent *event) override;

    void onFilterVisibilityChanged(obs_source_t *filter, bool enabled);
    void onFilterRenamed(obs_source_t *filter, const QString &oldName);
    QWidget *rowForFilter(obs_source_t *filter) const;

    void refreshExpandedFilters();

    void animateRowEntrance(QWidget *row, int delay);

    QVector<QString> lastRenderedNames;
    FilterCategory lastRenderedCategory = FilterCategory::Effect;
    bool renderedOnce = false;
};
