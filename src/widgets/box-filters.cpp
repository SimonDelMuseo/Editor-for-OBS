#include "box-filters.hpp"
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

    const int radius = 5;

    QRect r = rect().adjusted(1, 1, -1, -1);

    QColor bg = palette().color(QPalette::AlternateBase);
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r, radius, radius);

    p.setPen(QColor(80, 80, 80));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, radius, radius);
}
