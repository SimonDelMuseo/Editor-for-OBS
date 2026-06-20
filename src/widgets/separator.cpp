#include "separator.hpp"
#include <QSizePolicy>

Separator::Separator(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(6);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setStyleSheet("background-color: palette(midlight);");
}
