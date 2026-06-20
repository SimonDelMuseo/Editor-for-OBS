#pragma once

#include <obs.h>
#include <obs-source.h>

#include <QWidget>
#include <QLineEdit>

class QVBoxLayout;
class FiltersModel;
class FiltersState;

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

    // 🔥 NUEVA SEÑAL: cada vez que se crea un popout
    void popoutCreated(QWidget *w);

    // 🔥 NUEVAS SEÑALES PARA COPIAR / PEGAR
    void copyRequested(int index);
    void pasteRequested(int index);

private:
    FiltersModel *model = nullptr;
    QVBoxLayout *mainLayout = nullptr;

    FilterCategory currentCategory = FilterCategory::Effect;

    std::vector<obs_source_t*> effectFilters;
    std::vector<obs_source_t*> audioFilters;

    FiltersState *state = nullptr;

    void rebuildUI();
    bool eventFilter(QObject *obj, QEvent *event) override;

    void refreshExpandedFilters();
};
