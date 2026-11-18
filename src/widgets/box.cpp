#include "box.hpp"
#include "editor-ui-helpers.hpp"
#include <QPainter>
#include <QVBoxLayout>

Box::Box(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("E33_Box");

    setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);
}

void Box::paintEvent(QPaintEvent *event)
{
    const editorui::BoxBorder b = editorui::GetThemeBoxBorder();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int margin = qMax(1, b.width);
    const QRect r = rect().adjusted(margin, margin, -margin, -margin);
    const int radius =
        qBound(0, b.radius, qMin(r.width(), r.height()) / 2);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(b.color, b.width));
    if (radius > 0)
        p.drawRoundedRect(r, radius, radius);
    else
        p.drawRect(r);

    QFrame::paintEvent(event);
}
