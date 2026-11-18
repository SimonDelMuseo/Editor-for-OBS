#include "preview-source.hpp"

#include <QByteArray>
#include <QCloseEvent>
#include <QFile>
#include <QGuiApplication>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QSettings>
#include <QShowEvent>
#include <QWindow>

extern "C" {
#include <graphics/graphics.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
}

#include <algorithm>

PreviewSource::PreviewSource(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint |
                          Qt::CustomizeWindowHint | Qt::WindowTitleHint |
                          Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
    setAttribute(Qt::WA_NativeWindow);
    setWindowTitle(obs_module_text("PreviewTitle"));
    setMinimumSize(160, 90);
    restoreGeometryState();
}

PreviewSource::~PreviewSource()
{
    if (isOpen_ && source)
        obs_source_dec_showing(source);
    destroyDisplay();
    if (source)
        obs_source_release(source);
}

void PreviewSource::open(obs_source_t *src)
{
    setSource(src);
    isOpen_ = true;
    if (src)
        obs_source_inc_showing(src);
    show();
    raise();
    emit windowOpened();
}

void PreviewSource::setSource(obs_source_t *src)
{
    if (src == source)
        return;
    if (isOpen_ && source)
        obs_source_dec_showing(source);
    if (source)
        obs_source_release(source);
    source = src;
    if (source)
        obs_source_get_ref(source);
    if (isOpen_ && source)
        obs_source_inc_showing(source);

    if (source) {
        const QString title =
            QString::fromUtf8(obs_module_text("PreviewTitlePrefix")) +
            QStringLiteral(" ") +
            QString::fromUtf8(obs_source_get_name(source));
        if (windowTitle() != title)
            setWindowTitle(title);
    }
}

void PreviewSource::closeWindow()
{
    if (isVisible())
        close();
}

void PreviewSource::createDisplay()
{
    if (display)
        return;
    if (!windowHandle() || !windowHandle()->isExposed())
        return;

    const qreal dpr = devicePixelRatioF();

    gs_init_data info = {};
    info.cx = uint32_t(width() * dpr);
    info.cy = uint32_t(height() * dpr);
    info.format = GS_BGRA;
    info.zsformat = GS_ZS_NONE;
    info.window.hwnd = (void *)winId();

    display = obs_display_create(&info, 0xFF000000);
    if (display)
        obs_display_add_draw_callback(display, drawPreview, this);
}

void PreviewSource::destroyDisplay()
{
    if (!display)
        return;
    obs_display_remove_draw_callback(display, drawPreview, this);
    obs_display_destroy(display);
    display = nullptr;
}

void PreviewSource::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    createDisplay();
}

void PreviewSource::paintEvent(QPaintEvent *event)
{
    if (!display)
        createDisplay();
    QWidget::paintEvent(event);
}

void PreviewSource::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (display) {
        const qreal dpr = devicePixelRatioF();
        obs_display_resize(display, uint32_t(width() * dpr),
                           uint32_t(height() * dpr));
    }
}

void PreviewSource::closeEvent(QCloseEvent *event)
{
    storeGeometryState();
    if (source)
        obs_source_dec_showing(source);
    isOpen_ = false;
    destroyDisplay();
    QWidget::closeEvent(event);
    emit windowClosed();
}

void PreviewSource::storeGeometryState()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QStringLiteral("E33-Editor"),
                       QStringLiteral("PreviewWindow"));
    settings.setValue(QStringLiteral("geometry"),
                      saveGeometry());
}

void PreviewSource::clearStoredGeometry()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QStringLiteral("E33-Editor"),
                       QStringLiteral("PreviewWindow"));
    settings.clear();
    settings.sync();
    QFile::remove(settings.fileName());
}

void PreviewSource::restoreGeometryState()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QStringLiteral("E33-Editor"),
                       QStringLiteral("PreviewWindow"));
    const QByteArray geom =
        settings.value(QStringLiteral("geometry")).toByteArray();
    if (geom.isEmpty()) {
        QScreen *scr = nullptr;
        if (QWidget *main =
                static_cast<QWidget *>(obs_frontend_get_main_window()))
            scr = main->screen();
        if (!scr)
            scr = QGuiApplication::primaryScreen();
        if (scr)
            move(scr->availableGeometry().center() - rect().center());
        return;
    }
    if (!restoreGeometry(geom))
        restoreGeometry(QByteArray::fromBase64(geom));
}

void PreviewSource::drawPreview(void *data, uint32_t cx, uint32_t cy)
{
    auto *self = static_cast<PreviewSource *>(data);
    obs_source_t *src = self->source;
    if (!src)
        return;

    const uint32_t sw = std::max(obs_source_get_width(src), 1u);
    const uint32_t sh = std::max(obs_source_get_height(src), 1u);
    const float scale =
        std::min(float(cx) / float(sw), float(cy) / float(sh));
    const int nw = int(scale * float(sw));
    const int nh = int(scale * float(sh));
    const int x = int((float(cx) - scale * float(sw)) * 0.5f);
    const int y = int((float(cy) - scale * float(sh)) * 0.5f);

    gs_viewport_push();
    gs_projection_push();
    const bool previous = gs_set_linear_srgb(true);

    gs_ortho(0.0f, float(sw), 0.0f, float(sh), -100.0f, 100.0f);
    gs_set_viewport(x, y, nw, nh);
    obs_source_video_render(src);

    gs_set_linear_srgb(previous);
    gs_projection_pop();
    gs_viewport_pop();
}
