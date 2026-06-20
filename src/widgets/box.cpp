#include "box.hpp"
#include <QVBoxLayout>

Box::Box(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("E33_Box");
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

    setStyleSheet(
        "#E33_Box { "
        " border: 1px solid #808080; "
        " border-radius: 10px; "
        "}"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);
}
