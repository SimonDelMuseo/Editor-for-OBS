#pragma once

#include <QWidget>

extern "C" {
#include <obs.h>
}

class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QPropertyAnimation;
class QCheckBox;
class QPushButton;
class FiltersModel;
class FiltersProperRenderer;
class FilterNameEdit;

class FilterWindow : public QWidget {
    Q_OBJECT

public:
    explicit FilterWindow(QWidget *parent = nullptr);
    ~FilterWindow() override;

    void setModel(FiltersModel *model, int filterIndex);
    void updateValues();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    FiltersModel *model = nullptr;
    int index = -1;

    QVBoxLayout *mainLayout = nullptr;
    QScrollArea *scrollArea = nullptr;
    QWidget *bodyBox = nullptr;
    QPropertyAnimation *bodyAnim = nullptr;
    int  bodyHBefore = -1;
    bool inApplyCollapse = false;
    void applyCollapse(bool collapsed);
    FiltersProperRenderer *properRenderer = nullptr;

    QWidget *headerRow = nullptr;
    QCheckBox *btnCollapse = nullptr;
    QCheckBox *btnVisible = nullptr;
    FilterNameEdit *nameEdit = nullptr;
    QPushButton *btnReset = nullptr;

    obs_source_t *currentFilter = nullptr;

    obs_source_t *currentOwner = nullptr;

    void commitRename();

    void onFilterVisibilityChanged(obs_source_t *filter, bool enabled);
    void onFilterRenamed(obs_source_t *filter);

    static void native_enable_cb(void *data, calldata_t *cd);
    static void native_rename_cb(void *data, calldata_t *cd);
    static void native_owner_rename_cb(void *data, calldata_t *cd);

    static int liveCount;
    static void frontend_cb(enum obs_frontend_event event, void *data);
};
