#pragma once

#include <QFrame>
#include <QToolButton>

class QHBoxLayout;
class QCheckBox;
class QPushButton;

class FiltersHeader : public QFrame {
    Q_OBJECT

public:
    explicit FiltersHeader(QWidget *parent = nullptr);

    enum class FilterCategory {
        Effect,
        Audio
    };

    void setActiveCategory(FilterCategory category);

    FilterCategory activeCategory() const { return currentCategory; }

    void updateAudioButtonState(bool enabled);

    void disableAudioCategory();

    void setStatesEnabled(bool add, bool remove, bool copy, bool paste,
                          bool effects, bool audio);

signals:
    void addFilterRequested();
    void removeFilterRequested();
    void moveUpRequested();
    void moveDownRequested();
    void copyRequested();
    void pasteRequested();

    void expandCollapseAllRequested(bool collapsed);
    void visibilityAllRequested(bool visible);
    void selectAllRequested(bool selected);
    void popoutAllRequested();

    void categoryChanged(FilterCategory category);

private:
    QPushButton *btnAdd      = nullptr;
    QPushButton *btnRemove   = nullptr;
    QPushButton *btnCopy     = nullptr;
    QPushButton *btnPaste    = nullptr;

    QToolButton *btnEffects  = nullptr;
    QToolButton *btnAudio    = nullptr;

    QCheckBox   *btnExpand      = nullptr;
    QCheckBox   *btnVisibility  = nullptr;
    QCheckBox   *btnSelectAll   = nullptr;
    QPushButton *btnPopoutAll   = nullptr;

    FilterCategory currentCategory = FilterCategory::Effect;

    void setupButton(QToolButton *btn, const QString &icon, const QString &tooltip);
};
