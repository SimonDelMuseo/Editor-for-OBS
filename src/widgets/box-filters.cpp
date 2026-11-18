#include "box-filters.hpp"
#include "editor-ui-helpers.hpp"
#include <QPainter>
#include <QStyleOption>

BoxFilters::BoxFilters(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(false);
    setMinimumHeight(28);
}

void BoxFilters::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QStyleOption opt;
    opt.initFrom(this);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const editorui::BoxBorder b = editorui::GetThemeBoxBorder();

    const int margin = qMax(1, b.width);
    const QRect r = rect().adjusted(margin, margin, -margin, -margin);
    const int radius =
        qBound(0, b.radius, qMin(r.width(), r.height()) / 2);

    QColor bg = palette().color(QPalette::AlternateBase);
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    if (radius > 0)
        p.drawRoundedRect(r, radius, radius);
    else
        p.drawRect(r);

    p.setPen(QPen(b.color, b.width));
    p.setBrush(Qt::NoBrush);
    if (radius > 0)
        p.drawRoundedRect(r, radius, radius);
    else
        p.drawRect(r);
}
