#include "filters-window.hpp"
#include "filters-model.hpp"

#include "../filters-files/filters-proper-renderer.hpp"

#include <QVBoxLayout>
#include <QScrollArea>

extern "C" {
#include <obs.h>
#include <obs-frontend-api.h>
}

#include <atomic>

/* ────────────────────────────────────────────────
   FLAG GLOBAL — OBS SE ESTÁ CERRANDO
   ──────────────────────────────────────────────── */
static std::atomic_bool g_obsShuttingDown{false};

static void filterwindow_frontend_cb(enum obs_frontend_event event, void *)
{
    if (event == OBS_FRONTEND_EVENT_EXIT)
        g_obsShuttingDown = true;
}

/* ────────────────────────────────────────────────
   CONSTRUCTOR
   ──────────────────────────────────────────────── */
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
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(6);

    /* 🔹 Scroll vertical SOLO para la popout */
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    properRenderer = new FiltersProperRenderer(this);
    properRenderer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    scrollArea->setWidget(properRenderer);
    mainLayout->addWidget(scrollArea);

    obs_frontend_add_event_callback(filterwindow_frontend_cb, nullptr);
}

/* ────────────────────────────────────────────────
   SET MODEL
   ──────────────────────────────────────────────── */
void FilterWindow::setModel(FiltersModel *m, int filterIndex)
{
    model = m;
    index = filterIndex;

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
    setWindowTitle(QString::fromUtf8(name));

    /* 🔹 Mostrar propiedades */
    properRenderer->setFilter(filter);
}

/* ────────────────────────────────────────────────
   UPDATE VALUES (solo si se llama manualmente)
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   CLOSE EVENT
   ──────────────────────────────────────────────── */
void FilterWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
}
