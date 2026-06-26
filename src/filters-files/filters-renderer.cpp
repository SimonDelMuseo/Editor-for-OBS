#include "filters-renderer.hpp"
#include "filters-model.hpp"
#include "filters-window.hpp"
#include "filters-proper-renderer.hpp"
#include "filters-state.hpp"
#include "src/widgets/box-filters.hpp"
#include "editor-ui-helpers.hpp"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QStyle>
#include <QCheckBox>
#include <QEvent>
#include <QMessageBox>
#include <QTimer>

#include <obs-module.h>

/* ────────────────────────────────────────────────
   CONSTRUCTOR
   ──────────────────────────────────────────────── */
FiltersRenderer::FiltersRenderer(QWidget *parent)
    : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    state = new FiltersState(this); 
    
}

/* ────────────────────────────────────────────────
   SET MODEL
   ──────────────────────────────────────────────── */
void FiltersRenderer::setModel(FiltersModel *m)
{
    model = m;

    if (model) {
        connect(model, &FiltersModel::filtersChanged,
                this, &FiltersRenderer::rebuildUI,
                Qt::UniqueConnection);

        // RESET
        connect(this, &FiltersRenderer::resetFilterRequested,
                model, &FiltersModel::resetFilter,
                Qt::UniqueConnection);

        // 🔥 COPIAR
        connect(this, &FiltersRenderer::copyRequested,
                model, [this](int index) {
                    model->copyFilter(index);
                },
                Qt::UniqueConnection);

        // 🔥 PEGAR
        connect(this, &FiltersRenderer::pasteRequested,
                model, [this](int index) {
                    model->pasteFilter(index);
                },
                Qt::UniqueConnection);
    }

    rebuildUI();
}

/* ────────────────────────────────────────────────
   SET CATEGORY (nuevo)
   ──────────────────────────────────────────────── */
void FiltersRenderer::setCategory(FilterCategory c)
{
    currentCategory = c;
    rebuildUI();
}

/* ────────────────────────────────────────────────
   REFRESH
   ──────────────────────────────────────────────── */
void FiltersRenderer::refresh()
{
    rebuildUI();
}

/* ────────────────────────────────────────────────
   EVENT FILTER
   ──────────────────────────────────────────────── */
bool FiltersRenderer::eventFilter(QObject *obj, QEvent *event)
{
    QLabel *label = qobject_cast<QLabel*>(obj);
    if (!label)
        return QWidget::eventFilter(obj, event);

    QWidget *box = label->parentWidget();
    if (!box)
        return QWidget::eventFilter(obj, event);

    FilterNameEdit *edit = box->findChild<FilterNameEdit*>();
    QCheckBox *selectCheck = box->findChild<QCheckBox*>("filterSelectCheck");

    if (event->type() == QEvent::MouseButtonDblClick) {
        if (!edit)
            return false;

        label->setVisible(false);
        edit->setVisible(true);
        edit->setFocus();
        edit->selectAll();

        return true;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        if (selectCheck) {
            selectCheck->setChecked(true);

            const auto allChecks = this->findChildren<QCheckBox*>("filterSelectCheck");
            for (QCheckBox *chk : allChecks) {
                if (chk != selectCheck)
                    chk->setChecked(false);
            }
        }

        return true;
    }

    return QWidget::eventFilter(obj, event);
}

/* ────────────────────────────────────────────────
   REBUILD UI (modificado para separar listas)
   ──────────────────────────────────────────────── */
void FiltersRenderer::rebuildUI()
{
    if (!model)
        return;

    // 🔥 GUARDAR ESTADO POR NOMBRE
    QHash<QString, bool> expandedBefore;

    {
        auto all = model->getFilters();
        for (auto *f : all) {
            QString name = QString::fromUtf8(obs_source_get_name(f));
            expandedBefore[name] = state->isExpanded(name);
        }
    }

    // Limpiar layout
    QLayoutItem *child;
    while ((child = mainLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }

    effectFilters.clear();
    audioFilters.clear();

    auto filters = model->getFilters();

    for (auto *f : filters) {
        uint32_t flags = obs_source_get_output_flags(f);
        bool isAudio = (flags & OBS_SOURCE_AUDIO) != 0;

        if (isAudio)
            audioFilters.push_back(f);
        else
            effectFilters.push_back(f);
    }

    const auto &activeList =
        (currentCategory == FilterCategory::Effect)
            ? effectFilters
            : audioFilters;

    for (int index = 0; index < (int)activeList.size(); ++index) {

        obs_source_t *f = activeList[index];
        const char *name = obs_source_get_name(f);

        QWidget *container = new QWidget(this);
        auto *v = new QVBoxLayout(container);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(2);

        auto *box = new BoxFilters(container);
        auto *h = new QHBoxLayout(box);
        h->setContentsMargins(8, 4, 8, 4);
        h->setSpacing(6);

        auto *btnExpand = new QCheckBox(box);
        btnExpand->setProperty("class", "indicator-expand");
        btnExpand->setCheckable(true);
        btnExpand->setFixedSize(20, 20);

        QString filterName = QString::fromUtf8(obs_source_get_name(f));
        bool wasExpanded = expandedBefore.value(filterName, false);
        btnExpand->setChecked(!wasExpanded);

        auto *btnVisible = new QCheckBox(box);
        btnVisible->setProperty("class", "indicator-visibility");
        btnVisible->setCheckable(true);
        btnVisible->setFixedSize(20, 20);
        btnVisible->setChecked(obs_source_enabled(f));

        connect(btnVisible, &QCheckBox::toggled, this, [=](bool checked) {
            obs_source_set_enabled(f, checked);
        });

        auto *label = new QLabel(QString::fromUtf8(name), box);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        label->installEventFilter(this);

        auto *edit = new FilterNameEdit(QString::fromUtf8(name), box);
        edit->setVisible(false);
        edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto finishEditing = [=]() {
            QString newName = edit->text();
            obs_source_set_name(f, newName.toUtf8().constData());
            label->setText(newName);
            edit->setVisible(false);
            label->setVisible(true);
        };

        connect(edit, &QLineEdit::editingFinished, this, finishEditing);
        connect(edit, &FilterNameEdit::lostFocus, this, finishEditing);

        auto *btnReset = new QPushButton(box);
        btnReset->setObjectName("resetButton");
        btnReset->setProperty("class", "icon-reset");
        btnReset->setFixedSize(20, 20);
        btnReset->setIconSize(QSize(14, 14));
        editorui::SetButtonIcon(
            btnReset,
            editorui::GetIconPath("Reset.svg").c_str()
        );
        btnReset->setStyleSheet("background: transparent; border: none;");

        connect(btnReset, &QPushButton::clicked, this, [=]() {
            int realIndex = model->indexOfFilter(f);
            if (realIndex >= 0)
                emit resetFilterRequested(realIndex);
        });

        auto *btnUp = new QPushButton(box);
        btnUp->setProperty("class", "icon-up");
        btnUp->setFixedSize(20, 20);
        btnUp->setIconSize(QSize(14, 14));
        btnUp->setStyleSheet("background: transparent; border: none;");

        auto *btnDown = new QPushButton(box);
        btnDown->setProperty("class", "icon-down");
        btnDown->setFixedSize(20, 20);
        btnDown->setIconSize(QSize(14, 14));
        btnDown->setStyleSheet("background: transparent; border: none;");

        connect(btnUp, &QPushButton::clicked, this, [=]() {
            int realIndex = model->indexOfFilter(f);
            if (realIndex > 0)
                emit moveFilterUpRequested(realIndex);
        });

        connect(btnDown, &QPushButton::clicked, this, [=]() {
            int realIndex = model->indexOfFilter(f);
            if (realIndex >= 0)
                emit moveFilterDownRequested(realIndex);
        });

        // ---------------------------------------------------------
        // 🔥 SELECT CHECK (solo para selección desde header)
        // ---------------------------------------------------------
        auto *selectCheck = new QCheckBox(box);
        selectCheck->setObjectName("filterSelectCheck");
        selectCheck->setFixedSize(20, 20);

        auto *btnPopup = new QPushButton(box);
        btnPopup->setFlat(true);
        btnPopup->setFixedSize(20, 20);
        btnPopup->setObjectName("popoutButton");
        btnPopup->setIconSize(QSize(14, 14));
        editorui::SetButtonIcon(
            btnPopup,
            editorui::GetIconPath("Popout.svg").c_str()
        );
        btnPopup->setStyleSheet("QPushButton { background: transparent; padding: 0; border: none; }");

        connect(btnPopup, &QPushButton::clicked, this, [=]() {
            int realIndex = model->indexOfFilter(f);
            if (realIndex >= 0) {

                auto *popup = new FilterWindow();
                popup->setModel(model, realIndex);
                popup->show();

                emit popoutCreated(popup);

                btnExpand->setChecked(true);

                auto propsList = container->findChildren<FiltersProperRenderer*>();
                for (auto *p : propsList)
                    p->setVisible(false);

                selectCheck->setChecked(false);
            }
        });

        h->addWidget(btnExpand);
        h->addWidget(btnVisible);
        h->addWidget(label, 1);
        h->addWidget(edit, 1);
        h->addStretch();
        h->addWidget(btnReset);
        h->addWidget(btnUp);
        h->addWidget(btnDown);
        h->addWidget(selectCheck);
        h->addWidget(btnPopup);

        v->addWidget(box);

        auto *props = new FiltersProperRenderer(container);
        props->setVisible(wasExpanded);
        if (wasExpanded)
            props->setFilter(f);

        v->addWidget(props);

        connect(btnExpand, &QCheckBox::toggled, this, [=](bool checked) {

            bool expanded = !checked;

            QString filterName2 = QString::fromUtf8(obs_source_get_name(f));
            state->setExpanded(filterName2, expanded);

            if (expanded) {
                selectCheck->setChecked(true);

                const auto allChecks = this->findChildren<QCheckBox*>("filterSelectCheck");
                for (QCheckBox *chk : allChecks) {
                    if (chk != selectCheck)
                        chk->setChecked(false);
                }

                props->setVisible(true);
                props->setFilter(f);
                emit filterExpanded(index);

            } else {
                selectCheck->setChecked(false);
                props->setVisible(false);
                emit filterCollapsed(index);
            }
        });

        mainLayout->addWidget(container);
    }

    mainLayout->addStretch();
}

/* ────────────────────────────────────────────────
   EXPANDIR / COLAPSAR TODOS
   ──────────────────────────────────────────────── */
void FiltersRenderer::toggleAllExpanded(bool collapsed)
{
    const auto checks = findChildren<QCheckBox*>();
    for (QCheckBox *chk : checks) {
        if (chk->property("class") == "indicator-expand") {
            chk->setChecked(collapsed);
        }
    }
}

/* ────────────────────────────────────────────────
   MOSTRAR / OCULTAR TODOS
   ──────────────────────────────────────────────── */
void FiltersRenderer::setAllVisible(bool visible)
{
    const auto checks = findChildren<QCheckBox*>();
    for (QCheckBox *chk : checks) {
        if (chk->property("class") == "indicator-visibility") {
            chk->setChecked(visible);
        }
    }
}

/* ────────────────────────────────────────────────
   SELECCIONAR / DESELECCIONAR TODOS
   ──────────────────────────────────────────────── */
void FiltersRenderer::setAllSelected(bool selected)
{
    const auto checks = findChildren<QCheckBox*>();
    for (QCheckBox *chk : checks) {
        if (chk->objectName() == "filterSelectCheck") {
            chk->setChecked(selected);
        }
    }
}

/* ────────────────────────────────────────────────
   POPOUT ALL
   ──────────────────────────────────────────────── */
void FiltersRenderer::popoutAll()
{
    if (!model)
        return;

    // 1) Elegir la lista correcta según la categoría activa
    const auto &activeList =
        (currentCategory == FilterCategory::Effect)
            ? effectFilters
            : audioFilters;

    const size_t count = activeList.size();

    // 2) Abrir popouts SOLO de la categoría activa
    for (size_t i = 0; i < count; ++i) {
        auto *popup = new FilterWindow();
        popup->setModel(model, model->indexOfFilter(activeList[i]));
        popup->show();
    }

    // 3) Colapsar SOLO los filtros visibles en esta categoría
    const auto expandChecks = findChildren<QCheckBox*>();
    for (QCheckBox *chk : expandChecks) {
        if (chk->property("class") == "indicator-expand") {
            chk->setChecked(true); // true = colapsado
        }
    }

    // 4) Ocultar TODOS los proper renderers visibles
    const auto propsList = findChildren<FiltersProperRenderer*>();
    for (auto *p : propsList) {
        p->setVisible(false);
    }

    // 5) Deseleccionar todos los filtros
    const auto selectChecks = findChildren<QCheckBox*>("filterSelectCheck");
    for (auto *chk : selectChecks) {
        chk->setChecked(false);
    }
}

/* ────────────────────────────────────────────────
   ELIMINAR FILTROS SELECCIONADOS (con confirmación)
   ──────────────────────────────────────────────── */
void FiltersRenderer::removeSelectedFilters()
{
    if (!model)
        return;

    /* --------------------------------------------
       CONTAR FILTROS SELECCIONADOS
       -------------------------------------------- */
    const auto checks = findChildren<QCheckBox*>("filterSelectCheck");

    int selectedCount = 0;
    for (QCheckBox *chk : checks) {
        if (chk->isChecked())
            selectedCount++;
    }

    if (selectedCount == 0)
        return;

    /* --------------------------------------------
       OBTENER TEXTOS TRADUCIDOS (OBS MODULE LOCALE)
       -------------------------------------------- */
    QString message;

    if (selectedCount == 1) {
        message = QString::fromUtf8(obs_module_text("DeleteFilterConfirmSingle"));
    } else {
        message = QString::fromUtf8(obs_module_text("DeleteFilterConfirmMultiple"))
                      .arg(selectedCount);
    }

    // Línea adicional: acción irreversible
    message += "\n\n" + QString::fromUtf8(obs_module_text("DeleteFilterIrreversible"));

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QString::fromUtf8(obs_module_text("DeleteFilterTitle")),
        message,
        QMessageBox::Ok | QMessageBox::Cancel,
        QMessageBox::Cancel
    );

    if (reply != QMessageBox::Ok)
        return;

    /* --------------------------------------------
       BORRAR FILTROS (de atrás hacia delante)
       -------------------------------------------- */
    const auto &activeList =
        (currentCategory == FilterCategory::Effect)
            ? effectFilters
            : audioFilters;

    int count = checks.size();
    if ((int)activeList.size() < count)
        count = (int)activeList.size();

    for (int i = count - 1; i >= 0; --i) {
        if (!checks[i]->isChecked())
            continue;

        obs_source_t *f = activeList[i];
        int realIndex = model->indexOfFilter(f);

        if (realIndex >= 0)
            model->removeFilter((size_t)realIndex);
    }
}

/* ────────────────────────────────────────────────
   COPIAR PEGAR FILTROS
   ──────────────────────────────────────────────── */
void FiltersRenderer::copyFromHeader()
{
    if (!model)
        return;

    // Elegir lista según categoría activa
    const auto &activeList =
        (currentCategory == FilterCategory::Effect)
            ? effectFilters
            : audioFilters;

    auto checks = findChildren<QCheckBox*>("filterSelectCheck");

    int count = (int)activeList.size();
    if (checks.size() < count)
        count = (int)checks.size();

    // 🔥 NUEVO: limpiar buffer antes de copiar
    model->clearCopiedFilters();

    // 🔥 NUEVO: copiar TODOS los seleccionados
    for (int i = 0; i < count; ++i) {
        if (checks[i]->isChecked()) {
            obs_source_t *f = activeList[i];
            int realIndex = model->indexOfFilter(f);
            if (realIndex >= 0)
                emit copyRequested(realIndex);
        }
    }
}

void FiltersRenderer::pasteFromHeader()
{
    if (!model)
        return;

    const auto &activeList =
        (currentCategory == FilterCategory::Effect)
            ? effectFilters
            : audioFilters;

    auto checks = findChildren<QCheckBox*>("filterSelectCheck");

    int count = (int)activeList.size();
    if (checks.size() < count)
        count = (int)checks.size();

    // 🔥 NUEVO: si no hay filtros en la lista activa,
    // pegar igualmente al final de la lista global de filtros
    if (count == 0) {
        auto allFilters = model->getFilters();
        int realIndex = (int)allFilters.size(); // pegar al final (incluye caso 0 filtros)
        emit pasteRequested(realIndex);
        return;
    }

    int targetIndex = -1;

    // 1) Intentar con el filtro seleccionado
    for (int i = 0; i < count; ++i) {
        if (checks[i]->isChecked()) {
            targetIndex = i;
            break;
        }
    }

    // 2) Si no hay ninguno seleccionado, usar el último
    if (targetIndex == -1)
        targetIndex = count - 1;

    obs_source_t *f = activeList[targetIndex];
    int realIndex = model->indexOfFilter(f);
    if (realIndex >= 0)
        emit pasteRequested(realIndex);   // ahora el modelo pega TODOS los copiados
}
