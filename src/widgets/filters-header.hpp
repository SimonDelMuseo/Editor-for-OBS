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

    // Cambiar borde de botones según categoría activa
    void setActiveCategory(FilterCategory category);

    // 🔥 NUEVO — consultar categoría activa
    FilterCategory activeCategory() const { return currentCategory; }

    // Activar/desactivar botón AudioFilters
    void updateAudioButtonState(bool enabled);

    void disableAudioCategory();

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

    // Cambio de categoría (Effect / Audio)
    void categoryChanged(FilterCategory category);

private:
    // Línea 1 (todos QToolButton)
    QPushButton *btnAdd      = nullptr;
    QPushButton *btnRemove   = nullptr;
    QPushButton *btnCopy     = nullptr;
    QPushButton *btnPaste    = nullptr;

    // Línea 2 — botones de texto (QToolButton)
    QToolButton *btnEffects  = nullptr;
    QToolButton *btnAudio    = nullptr;

    // Línea 2 — botones tipo OBS (QCheckBox / QPushButton)
    QCheckBox   *btnExpand      = nullptr;
    QCheckBox   *btnVisibility  = nullptr;
    QCheckBox   *btnSelectAll   = nullptr;
    QPushButton *btnPopoutAll   = nullptr;

    // 🔥 NUEVO — estado interno de la categoría actual
    FilterCategory currentCategory = FilterCategory::Effect;

    void setupButton(QToolButton *btn, const QString &icon, const QString &tooltip);
};
