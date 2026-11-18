#include "filters-window.hpp"
#include "filters-model.hpp"
#include "filters-renderer.hpp"
#include "../filters-files/filters-proper-renderer.hpp"
#include "editor-ui-helpers.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPainter>
#include <QPropertyAnimation>
#include <QCheckBox>
#include <QPushButton>
#include <QApplication>
#include <QScreen>

extern "C" {
#include <obs.h>
#include <obs-frontend-api.h>
}

#include <atomic>

static std::atomic_bool g_obsShuttingDown{false};

static constexpr int kMinHeightExpanded = 300;

int FilterWindow::liveCount = 0;

void FilterWindow::frontend_cb(enum obs_frontend_event event, void *)
{
    if (event == OBS_FRONTEND_EVENT_EXIT)
        g_obsShuttingDown = true;
}


static QPoint nextCascadePos()
{
    static int s_index = 0;

    int visible = 0;
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (qobject_cast<FilterWindow *>(w) && w->isVisible())
            ++visible;
    }
    if (visible == 0)
        s_index = 0;
    else if (s_index > 19)
        s_index = 0;

    QRect screen(0, 0, 1920, 1080);
    QPoint anchor(120, 120);
    if (auto *obsMain = static_cast<QWidget *>(
            obs_frontend_get_main_window())) {
        screen = obsMain->screen()->availableGeometry();
        anchor = screen.center() - QPoint(225, 250);
    }

    QPoint pos = anchor + QPoint(s_index * 30, s_index * 30);
    pos.setX(qMin(pos.x(), screen.right() - 450));
    pos.setY(qMin(pos.y(), screen.bottom() - 400));
    ++s_index;
    return pos;
}

static void applyNameEditEnabled(QLineEdit *edit, bool enabled)
{
    if (!enabled) {
        const QString gray = QStringLiteral("color: #808080;");
        if (edit->styleSheet() != gray)
            edit->setStyleSheet(gray);
        return;
    }
    if (!edit->styleSheet().isEmpty())
        edit->setStyleSheet(QString());
}

namespace {
class PopoutBar : public QWidget
{
public:
    explicit PopoutBar(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAutoFillBackground(false);
        setMinimumHeight(28);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setPen(QPen(editorui::GetPanelLineColor(), 1));
        p.drawLine(0, 0, width(), 0);
        const int y = height() - 1;
        p.drawLine(0, y, width(), y);
    }
};
}

FilterWindow::FilterWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlag(Qt::Window);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::CustomizeWindowHint);
    setWindowFlag(Qt::WindowTitleHint);
    setWindowFlag(Qt::WindowCloseButtonHint);
    setWindowFlag(Qt::WindowMinimizeButtonHint);

    setMinimumSize(400, 300);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 12);
    mainLayout->setSpacing(6);

    headerRow = new PopoutBar(this);
    headerRow->setObjectName("popoutHeader");
    headerRow->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    headerRow->setEnabled(false);

    auto *h = new QHBoxLayout(headerRow);
    h->setContentsMargins(8, 0, 8, 4);
    h->setSpacing(6);

    btnCollapse = new QCheckBox(headerRow);
    btnCollapse->setObjectName("popoutCollapse");
    btnCollapse->setProperty("class", "indicator-expand");
    btnCollapse->setCheckable(true);
    btnCollapse->setFixedSize(20, 20);
    btnCollapse->setChecked(false);

    btnVisible = new QCheckBox(headerRow);
    btnVisible->setObjectName("popoutVisible");
    btnVisible->setProperty("class", "indicator-visibility");
    btnVisible->setCheckable(true);
    btnVisible->setFixedSize(20, 20);

    nameEdit = new FilterNameEdit(headerRow);
    nameEdit->setObjectName("popoutNameEdit");
    nameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    btnReset = new QPushButton(headerRow);
    btnReset->setObjectName("popoutReset");
    btnReset->setProperty("class", "icon-reset");
    btnReset->setFixedSize(20, 20);
    btnReset->setIconSize(QSize(14, 14));
    editorui::SetButtonIcon(
        btnReset,
        editorui::GetIconPath("Reset.svg").c_str()
    );
    btnReset->setStyleSheet("background: transparent; border: none;");

    h->addWidget(btnCollapse);
    h->addWidget(btnVisible);
    h->addWidget(nameEdit, 1);
    h->addWidget(btnReset);

    mainLayout->addWidget(headerRow);

    bodyBox = new QWidget(this);
    auto *bodyLayout = new QVBoxLayout(bodyBox);
    bodyLayout->setContentsMargins(12, 0, 12, 0);
    bodyLayout->setSpacing(0);

    scrollArea = new QScrollArea(bodyBox);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    properRenderer = new FiltersProperRenderer(this);
    properRenderer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    scrollArea->setWidget(properRenderer);
    bodyLayout->addWidget(scrollArea);
    mainLayout->addWidget(bodyBox);

    bodyAnim = new QPropertyAnimation(bodyBox, "maximumHeight", this);
    bodyAnim->setDuration(250);
    bodyAnim->setEasingCurve(QEasingCurve::OutSine);

    connect(bodyAnim, &QPropertyAnimation::valueChanged, this,
            [this](const QVariant &v) {
        const int want = headerRow->sizeHint().height()
                         + mainLayout->spacing() + v.toInt() + 12;
        resize(width(), want);
    });

    connect(bodyAnim, &QPropertyAnimation::finished, this, [this]() {
        if (inApplyCollapse)
            return;
        if (btnCollapse->isChecked()) {
            bodyBox->hide();
            bodyBox->setMaximumHeight(0);
            const int want = headerRow->sizeHint().height()
                             + mainLayout->spacing() + 12;
            setMinimumHeight(want);
            resize(width(), want);
        } else {
            bodyBox->setMaximumHeight(QWIDGETSIZE_MAX);
            const int want = qMax(headerRow->sizeHint().height()
                                      + mainLayout->spacing()
                                      + bodyHBefore + 12,
                                  kMinHeightExpanded);
            setMinimumHeight(kMinHeightExpanded);
            mainLayout->activate();
            resize(width(), want);
        }
    });

    connect(btnCollapse, &QCheckBox::toggled, this,
            [this](bool c) { applyCollapse(c); });


    connect(btnVisible, &QCheckBox::toggled, this, [this](bool checked) {
        if (!currentFilter || !model)
            return;
        const int i = model->indexOfFilter(currentFilter);
        if (i < 0)
            return;
        model->setFilterVisibility(i, checked);
        applyNameEditEnabled(nameEdit, checked);
    });

    connect(nameEdit, &FilterNameEdit::editingFinished,
            this, &FilterWindow::commitRename);
    connect(nameEdit, &FilterNameEdit::lostFocus,
            this, &FilterWindow::commitRename);

    connect(btnReset, &QPushButton::clicked, this, [this]() {
        if (!currentFilter || !model)
            return;
        const int i = model->indexOfFilter(currentFilter);
        if (i < 0)
            return;
        model->resetFilter(i);
    });

    if (liveCount == 0)
        obs_frontend_add_event_callback(FilterWindow::frontend_cb, nullptr);
    liveCount++;

    move(nextCascadePos());
}

FilterWindow::~FilterWindow()
{
    if (currentFilter) {
        signal_handler_disconnect(
            obs_source_get_signal_handler(currentFilter), "enable",
            FilterWindow::native_enable_cb, this);
        signal_handler_disconnect(
            obs_source_get_signal_handler(currentFilter), "rename",
            FilterWindow::native_rename_cb, this);
        obs_source_release(currentFilter);
        currentFilter = nullptr;
    }
    if (currentOwner) {
        signal_handler_disconnect(
            obs_source_get_signal_handler(currentOwner), "rename",
            FilterWindow::native_owner_rename_cb, this);
        obs_source_release(currentOwner);
        currentOwner = nullptr;
    }

    if (liveCount > 0) {
        liveCount--;
        if (liveCount == 0)
            obs_frontend_remove_event_callback(FilterWindow::frontend_cb,
                                               nullptr);
    }
}

void FilterWindow::applyCollapse(bool collapsed)
{
    inApplyCollapse = true;
    if (bodyAnim->state() == QAbstractAnimation::Running)
        bodyAnim->stop();
    inApplyCollapse = false;

    if (collapsed) {
        bodyBox->setMaximumHeight(QWIDGETSIZE_MAX);
        bodyBox->show();
        const int natural = bodyBox->height() > 0
                                ? bodyBox->height()
                                : bodyBox->sizeHint().height();
        bodyHBefore = natural;

        setMinimumHeight(0);
        bodyAnim->setStartValue(bodyBox->height() > 0 ? bodyBox->height()
                                                      : natural);
        bodyAnim->setEndValue(0);
        bodyAnim->start();
    } else {
        int natural = bodyHBefore;
        if (natural <= 0) {
            bodyBox->setMaximumHeight(QWIDGETSIZE_MAX);
            natural = bodyBox->sizeHint().height();
        }
        bodyBox->setMaximumHeight(0);
        bodyBox->show();

        const int base = headerRow->sizeHint().height()
                         + mainLayout->spacing() + 12;
        setMinimumHeight(0);
        resize(width(), base);

        bodyAnim->setStartValue(0);
        bodyAnim->setEndValue(natural > 0 ? natural : 1);
        bodyAnim->start();
    }
}

void FilterWindow::setModel(FiltersModel *m, int filterIndex)
{
    model = m;
    index = filterIndex;

    if (model) {
        connect(model, &FiltersModel::filterVisibilityChanged,
                this, &FilterWindow::onFilterVisibilityChanged,
                Qt::UniqueConnection);
        connect(model, &FiltersModel::filterRenamed,
                this, &FilterWindow::onFilterRenamed,
                Qt::UniqueConnection);
    }

    if (currentFilter) {
        signal_handler_disconnect(
            obs_source_get_signal_handler(currentFilter), "enable",
            FilterWindow::native_enable_cb, this);
        signal_handler_disconnect(
            obs_source_get_signal_handler(currentFilter), "rename",
            FilterWindow::native_rename_cb, this);
        obs_source_release(currentFilter);
        currentFilter = nullptr;
    }
    if (currentOwner) {
        signal_handler_disconnect(
            obs_source_get_signal_handler(currentOwner), "rename",
            FilterWindow::native_owner_rename_cb, this);
        obs_source_release(currentOwner);
        currentOwner = nullptr;
    }

    headerRow->setEnabled(false);

    if (!model || index < 0) {
        properRenderer->setFilter(nullptr);
        return;
    }

    auto filters = model->getFilters();
    if (index >= (int)filters.size()) {
        properRenderer->setFilter(nullptr);
        return;
    }

    obs_source_t *filter = filters[index];
    if (!filter) {
        properRenderer->setFilter(nullptr);
        return;
    }

    const char *name = obs_source_get_name(filter);

    obs_source_t *owner = model->getSource();
    if (owner) {
        if (const char *srcName = obs_source_get_name(owner))
            setWindowTitle(QString::fromUtf8(srcName));
        currentOwner = obs_source_get_ref(owner);
        signal_handler_connect(
            obs_source_get_signal_handler(currentOwner), "rename",
            FilterWindow::native_owner_rename_cb, this);
    } else {
        setWindowTitle(QString::fromUtf8(name));
    }

    currentFilter = obs_source_get_ref(filter);
    nameEdit->setText(QString::fromUtf8(name));
    const bool enabled = obs_source_enabled(filter);
    btnVisible->setChecked(enabled);
    applyNameEditEnabled(nameEdit, enabled);
    headerRow->setEnabled(true);

    signal_handler_connect(
        obs_source_get_signal_handler(currentFilter), "enable",
        FilterWindow::native_enable_cb, this);
    signal_handler_connect(
        obs_source_get_signal_handler(currentFilter), "rename",
        FilterWindow::native_rename_cb, this);

    properRenderer->setFilter(filter);
}

void FilterWindow::commitRename()
{
    if (!currentFilter || !model) {
        nameEdit->clearFocus();
        return;
    }
    const int i = model->indexOfFilter(currentFilter);
    if (i < 0) {
        nameEdit->clearFocus();
        return;
    }

    QString newName = nameEdit->text().trimmed();
    if (newName.isEmpty())
        newName = QString::fromUtf8(obs_source_get_name(currentFilter));
    if (nameEdit->text() != newName)
        nameEdit->setText(newName);

    model->renameFilter(i, newName.toStdString());

    nameEdit->clearFocus();
}

void FilterWindow::onFilterVisibilityChanged(obs_source_t *filter,
                                             bool enabled)
{
    if (filter != currentFilter)
        return;
    if (btnVisible->isChecked() != enabled)
        btnVisible->setChecked(enabled);
}

void FilterWindow::onFilterRenamed(obs_source_t *filter)
{
    if (filter != currentFilter)
        return;
    if (nameEdit->hasFocus())
        return;
    if (const char *n = obs_source_get_name(filter))
        nameEdit->setText(QString::fromUtf8(n));
}

void FilterWindow::native_enable_cb(void *data, calldata_t *cd)
{
    auto *self = static_cast<FilterWindow *>(data);
    obs_source_t *src =
        static_cast<obs_source_t *>(calldata_ptr(cd, "source"));
    const bool enabled = calldata_bool(cd, "enabled");

    QMetaObject::invokeMethod(
        self,
        [self, src, enabled]() {
            if (!self->currentFilter || src != self->currentFilter)
                return;
            if (self->btnVisible->isChecked() == enabled) {
                applyNameEditEnabled(self->nameEdit, enabled);
                return;
            }
            self->btnVisible->setChecked(enabled);
        },
        Qt::QueuedConnection);
}

void FilterWindow::native_rename_cb(void *data, calldata_t *cd)
{
    auto *self = static_cast<FilterWindow *>(data);
    obs_source_t *src =
        static_cast<obs_source_t *>(calldata_ptr(cd, "source"));
    const char *newName = calldata_string(cd, "new_name");
    const QString nameQ =
        newName ? QString::fromUtf8(newName) : QString();

    QMetaObject::invokeMethod(
        self,
        [self, src, nameQ]() {
            if (!self->currentFilter || src != self->currentFilter)
                return;
            if (self->nameEdit->hasFocus())
                return;
            if (!nameQ.isEmpty())
                self->nameEdit->setText(nameQ);
        },
        Qt::QueuedConnection);
}

void FilterWindow::native_owner_rename_cb(void *data, calldata_t *cd)
{
    auto *self = static_cast<FilterWindow *>(data);
    obs_source_t *src =
        static_cast<obs_source_t *>(calldata_ptr(cd, "source"));
    const char *newName = calldata_string(cd, "new_name");
    const QString nameQ =
        newName ? QString::fromUtf8(newName) : QString();

    QMetaObject::invokeMethod(
        self,
        [self, src, nameQ]() {
            if (!self->currentOwner || src != self->currentOwner)
                return;
            if (!nameQ.isEmpty())
                self->setWindowTitle(nameQ);
        },
        Qt::QueuedConnection);
}

void FilterWindow::updateValues()
{
    if (g_obsShuttingDown.load())
        return;

    if (!model || index < 0)
        return;

    auto filters = model->getFilters();
    if (index >= (int)filters.size())
        return;

    obs_source_t *filter = filters[index];
    if (!filter)
        return;

    properRenderer->updateValues();
}

void FilterWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
}
