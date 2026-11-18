#include "filters-copy-paste.hpp"
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
#include <QApplication>
#include <QPropertyAnimation>
#include <QPointer>

#include <obs-module.h>

FiltersRenderer::FiltersRenderer(QWidget *parent)
    : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    state = new FiltersState(this); 
    
}

void FiltersRenderer::setModel(FiltersModel *m)
{
    model = m;

    if (model) {
        connect(model, &FiltersModel::filtersChanged,
                this, &FiltersRenderer::rebuildUI,
                Qt::UniqueConnection);

        connect(this, &FiltersRenderer::resetFilterRequested,
                model, &FiltersModel::resetFilter,
                Qt::UniqueConnection);

        connect(this, &FiltersRenderer::copyRequested,
                model, [this](int index) {
                    model->copyFilter(index);
                },
                Qt::UniqueConnection);

        connect(this, &FiltersRenderer::pasteRequested,
                model, [this](int index) {
                    model->pasteFilter(index);
                },
                Qt::UniqueConnection);

        connect(model, &FiltersModel::filterVisibilityChanged,
                this, &FiltersRenderer::onFilterVisibilityChanged,
                Qt::UniqueConnection);
        connect(model, &FiltersModel::filterRenamed,
                this, &FiltersRenderer::onFilterRenamed,
                Qt::UniqueConnection);
    }

    rebuildUI();
}

void FiltersRenderer::setCategory(FilterCategory c)
{
    currentCategory = c;
    rebuildUI();
}

void FiltersRenderer::animateRowEntrance(QWidget *row, int delay)
{
    const int target = row->sizeHint().height();
    if (target <= 0)
        return;

    row->setMaximumHeight(0);

    auto *anim = new QPropertyAnimation(row, "maximumHeight", row);
    anim->setDuration(250);
    anim->setStartValue(0);
    anim->setEndValue(target);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(anim, &QPropertyAnimation::finished,
            row, [row]() { row->setMaximumHeight(QWIDGETSIZE_MAX); });

    if (delay <= 0) {
        anim->start();
    } else {
        QTimer::singleShot(delay, row,
            [animPtr = QPointer<QPropertyAnimation>(anim)]() {
                if (!animPtr.isNull())
                    animPtr->start();
            });
    }
}

void FiltersRenderer::refresh()
{
    rebuildUI();
}

static void applyFilterNameVisibility(QLabel *label, bool enabled)
{
    label->setProperty("filterEnabled", enabled);

    if (!enabled) {
        const QString gray = QStringLiteral("color: #808080;");
        if (label->styleSheet() != gray)
            label->setStyleSheet(gray);
        return;
    }

    const QColor color =
        QApplication::palette().color(QPalette::Active, QPalette::WindowText);
    const QString css =
        QStringLiteral("color: %1;").arg(color.name());
    if (label->styleSheet() == css)
        return;
    label->setStyleSheet(css);
}

bool FiltersRenderer::eventFilter(QObject *obj, QEvent *event)
{
    QLabel *label = qobject_cast<QLabel*>(obj);
    if (!label)
        return QWidget::eventFilter(obj, event);

    if (event->type() == QEvent::PaletteChange) {
        if (label->property("filterEnabled").isValid() &&
            !label->property("filterApplying").toBool()) {
            label->setProperty("filterApplying", true);
            applyFilterNameVisibility(
                label, label->property("filterEnabled").toBool());
            label->setProperty("filterApplying", false);
        }
        return QWidget::eventFilter(obj, event);
    }

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

void FiltersRenderer::rebuildUI()
{
    if (!model)
        return;

    if (renderedOnce && currentCategory == lastRenderedCategory) {
        QVector<QString> now;
        const auto all = model->getFilters();
        for (auto *f : all) {
            const uint32_t flags = obs_source_get_output_flags(f);
            const bool isAudio = (flags & OBS_SOURCE_AUDIO) != 0;
            if ((currentCategory == FilterCategory::Audio) == isAudio)
                now.append(QString::fromUtf8(obs_source_get_name(f)));
        }
        if (now == lastRenderedNames)
            return;
    }

    QHash<QString, bool> expandedBefore;

    const QVector<QString> renderedBefore = lastRenderedNames;

    {
        auto all = model->getFilters();
        for (auto *f : all) {
            QString name = QString::fromUtf8(obs_source_get_name(f));
            expandedBefore[name] = state->isExpanded(name);
        }
    }

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

    int newRows = 0;

    for (int index = 0; index < (int)activeList.size(); ++index) {

        obs_source_t *f = activeList[index];
        const char *name = obs_source_get_name(f);

        QWidget *container = new QWidget(this);
        container->setProperty("filterAddr",
                               reinterpret_cast<qulonglong>(f));
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

        auto *label = new QLabel(QString::fromUtf8(name), box);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        label->installEventFilter(this);

        applyFilterNameVisibility(label, obs_source_enabled(f));

        connect(btnVisible, &QCheckBox::toggled, this, [=](bool checked) {
            if (!model)
                return;
            const int realIndex = model->indexOfFilter(f);
            if (realIndex < 0)
                return;
            model->setFilterVisibility(realIndex, checked);
        });

        auto *edit = new FilterNameEdit(QString::fromUtf8(name), box);
        edit->setVisible(false);
        edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto finishEditing = [=]() {
            QString newName = edit->text().trimmed();
            if (newName.isEmpty()) {
                newName = QString::fromUtf8(obs_source_get_name(f));
                edit->setText(newName);
            }
            if (model) {
                const int realIndex = model->indexOfFilter(f);
                if (realIndex >= 0)
                    model->renameFilter(realIndex, newName.toStdString());
            }
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

        auto *selectCheck = new QCheckBox(box);
        selectCheck->setObjectName("filterSelectCheck");
        selectCheck->setFixedSize(20, 20);

        connect(selectCheck, &QCheckBox::toggled, this,
                [this](bool) { emit selectionStateChanged(); });

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

        if (!renderedBefore.contains(QString::fromUtf8(name)))
            animateRowEntrance(container, newRows++ * 40);
    }

    mainLayout->addStretch();

    lastRenderedNames.clear();
    for (auto *f : activeList)
        lastRenderedNames.append(QString::fromUtf8(obs_source_get_name(f)));
    lastRenderedCategory = currentCategory;
    renderedOnce = true;

    emit selectionStateChanged();
}

void FiltersRenderer::markExpanded(const QString &name)
{
    state->setExpanded(name, true);
}

void FiltersRenderer::toggleAllExpanded(bool collapsed)
{
    const auto checks = findChildren<QCheckBox*>();
    for (QCheckBox *chk : checks) {
        if (chk->property("class") == "indicator-expand") {
            chk->setChecked(collapsed);
        }
    }
}

void FiltersRenderer::setAllVisible(bool visible)
{
    const auto checks = findChildren<QCheckBox*>();
    for (QCheckBox *chk : checks) {
        if (chk->property("class") == "indicator-visibility") {
            chk->setChecked(visible);
        }
    }
}

void FiltersRenderer::setAllSelected(bool selected)
{
    const auto checks = findChildren<QCheckBox*>();
    for (QCheckBox *chk : checks) {
        if (chk->objectName() == "filterSelectCheck") {
            chk->setChecked(selected);
        }
    }
}

void FiltersRenderer::popoutAll()
{
    if (!model)
        return;

    const auto &activeList =
        (currentCategory == FilterCategory::Effect)
            ? effectFilters
            : audioFilters;

    for (obs_source_t *f : activeList) {
        auto *popup = new FilterWindow();
        popup->setModel(model, model->indexOfFilter(f));
        popup->show();

        emit popoutCreated(popup);
    }

    const auto expandChecks = findChildren<QCheckBox*>();
    for (QCheckBox *chk : expandChecks) {
        if (chk->property("class") == "indicator-expand") {
            chk->setChecked(true);
        }
    }

    const auto propsList = findChildren<FiltersProperRenderer*>();
    for (auto *p : propsList) {
        p->setVisible(false);
    }

    const auto selectChecks = findChildren<QCheckBox*>("filterSelectCheck");
    for (auto *chk : selectChecks) {
        chk->setChecked(false);
    }
}

void FiltersRenderer::removeSelectedFilters()
{
    if (!model)
        return;

    const auto checks = findChildren<QCheckBox*>("filterSelectCheck");

    int selectedCount = 0;
    for (QCheckBox *chk : checks) {
        if (chk->isChecked())
            selectedCount++;
    }

    if (selectedCount == 0)
        return;

    QString message;

    if (selectedCount == 1) {
        message = QString::fromUtf8(obs_module_text("DeleteFilterConfirmSingle"));
    } else {
        message = QString::fromUtf8(obs_module_text("DeleteFilterConfirmMultiple"))
                      .arg(selectedCount);
    }

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

QWidget *FiltersRenderer::rowForFilter(obs_source_t *filter) const
{
    const qulonglong addr = reinterpret_cast<qulonglong>(filter);
    for (int i = 0; i < mainLayout->count(); ++i) {
        QLayoutItem *item = mainLayout->itemAt(i);
        QWidget *w = item ? item->widget() : nullptr;
        if (w && w->property("filterAddr").toULongLong() == addr)
            return w;
    }
    return nullptr;
}

void FiltersRenderer::onFilterVisibilityChanged(obs_source_t *filter,
                                                bool enabled)
{
    QWidget *container = rowForFilter(filter);
    if (!container)
        return;

    for (QCheckBox *chk : container->findChildren<QCheckBox *>()) {
        if (chk->property("class") == "indicator-visibility") {
            if (chk->isChecked() != enabled)
                chk->setChecked(enabled);
            break;
        }
    }

    if (QLabel *label = container->findChild<QLabel *>())
        applyFilterNameVisibility(label, enabled);
}

void FiltersRenderer::onFilterRenamed(obs_source_t *filter,
                                      const QString &oldName)
{
    const int idx = (model && filter) ? model->indexOfFilter(filter) : -1;
    QWidget *container = rowForFilter(filter);
    if (idx < 0)
        return;

    const QString newName =
        QString::fromUtf8(obs_source_get_name(filter));

    if (state)
        state->rename(oldName, newName);
    for (QString &n : lastRenderedNames) {
        if (n == oldName) {
            n = newName;
            break;
        }
    }

    if (!container)
        return;

    if (QLabel *label = container->findChild<QLabel *>())
        label->setText(newName);
    if (FilterNameEdit *edit = container->findChild<FilterNameEdit *>()) {
        if (!edit->isVisible())
            edit->setText(newName);
    }
}

void FiltersRenderer::copyFromHeader()
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

    clearCopiedFilters();

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

    if (count == 0) {
        auto allFilters = model->getFilters();
        int realIndex = (int)allFilters.size();
        emit pasteRequested(realIndex);
        return;
    }

    int targetIndex = -1;

    for (int i = 0; i < count; ++i) {
        if (checks[i]->isChecked()) {
            targetIndex = i;
            break;
        }
    }

    if (targetIndex == -1)
        targetIndex = count - 1;

    obs_source_t *f = activeList[targetIndex];
    int realIndex = model->indexOfFilter(f);
    if (realIndex >= 0)
        emit pasteRequested(realIndex);
}
