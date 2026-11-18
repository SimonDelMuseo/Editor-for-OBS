#include "e33-spinbox.hpp"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QWheelEvent>

E33DoubleSpinBox::E33DoubleSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent)
{
    setKeyboardTracking(false);
    setFocusPolicy(Qt::StrongFocus);
}

void E33DoubleSpinBox::keyPressEvent(QKeyEvent *event)
{
    const bool shift = (event->modifiers() & Qt::ShiftModifier) != 0;
    switch (event->key()) {
    case Qt::Key_Up:
        event->accept();
        if (stepEnabled() & StepUpEnabled)
            stepBy(shift ? 10 : 1);
        return;
    case Qt::Key_Down:
        event->accept();
        if (stepEnabled() & StepDownEnabled)
            stepBy(shift ? -10 : -1);
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        event->accept();
        interpretText();
        return;
    }
    QDoubleSpinBox::keyPressEvent(event);
}

void E33DoubleSpinBox::wheelEvent(QWheelEvent *event)
{
    if (!hasFocus())
        event->ignore();
    else
        QDoubleSpinBox::wheelEvent(event);
}

void E33DoubleSpinBox::focusInEvent(QFocusEvent *event)
{
    QDoubleSpinBox::focusInEvent(event);
    lineEdit()->selectAll();
}
