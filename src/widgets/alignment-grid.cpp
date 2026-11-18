#include "alignment-grid.hpp"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionFocusRect>
#include <QStyleOptionFrame>

#include <algorithm>

AlignmentSelector::AlignmentSelector(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
}

QSize AlignmentSelector::sizeHint() const
{
    int base = fontMetrics().height() * 2;
    return QSize(base, base);
}

QSize AlignmentSelector::minimumSizeHint() const
{
    return QSize(16, 16);
}

Qt::Alignment AlignmentSelector::value() const
{
    return cellAlignment(selectedCell);
}

int AlignmentSelector::currentIndex() const
{
    return selectedCell;
}

void AlignmentSelector::setSelected(int index)
{
    if (index < 0 || index > 8)
        return;
    selectedCell = index;
    focusedCell  = index;
    update();
}

void AlignmentSelector::setButtonsEnabled(bool enabled)
{
    setEnabled(enabled);
    update();
}

void AlignmentSelector::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QStyle *style = this->style();

    const int cellW = gridRect().width() / 3;
    const int cellH = gridRect().height() / 3;

    for (int i = 0; i < 9; ++i) {
        QRect rect = cellRect(i);
        rect = rect.adjusted(0, 0, -1, -1);

        QStyleOptionFrame frameOpt;
        frameOpt.rect = rect;
        frameOpt.state = isEnabled() ? QStyle::State_Enabled
                                     : QStyle::State_None;
        frameOpt.lineWidth = 1;
        frameOpt.midLineWidth = 0;
        if (i == hoveredCell)
            frameOpt.state |= QStyle::State_MouseOver;
        if (i == selectedCell)
            frameOpt.state |= QStyle::State_On;
        if (i == focusedCell && hasFocus())
            frameOpt.state |= QStyle::State_HasFocus;

        QStyleOptionButton radioOpt;
        radioOpt.state = isEnabled() ? QStyle::State_Enabled
                                     : QStyle::State_None;
        radioOpt.rect = rect.adjusted(cellW / 6, cellH / 6,
                                      -cellW / 6, -cellH / 6);
        if (i == hoveredCell)
            radioOpt.state |= QStyle::State_MouseOver;
        if (i == selectedCell)
            radioOpt.state |= QStyle::State_On;
        if (i == focusedCell && hasFocus())
            radioOpt.state |= QStyle::State_HasFocus;

        style->drawPrimitive(QStyle::PE_IndicatorRadioButton,
                             &radioOpt, &painter, this);
        style->drawPrimitive(QStyle::PE_Frame, &frameOpt, &painter, this);

        if (i == focusedCell && hasFocus()) {
            QStyleOptionFocusRect focusOpt;
            focusOpt.initFrom(this);
            focusOpt.rect = rect.adjusted(1, 1, -1, -1);
            focusOpt.state = isEnabled() ? QStyle::State_Enabled
                                         : QStyle::State_None;
            focusOpt.state |= QStyle::State_HasFocus;
            style->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOpt,
                                 &painter, this);
        }
    }
}


QRect AlignmentSelector::cellRect(int index) const
{
    const int col = index % 3;
    const int row = index / 3;

    const QRect grid = gridRect();
    const int cellW = grid.width() / 3;
    const int cellH = grid.height() / 3;

    return QRect(col * cellW + grid.left(),
                 row * cellH + grid.top(), cellW, cellH);
}

Qt::Alignment AlignmentSelector::cellAlignment(int index) const
{
    Qt::Alignment hAlign;
    Qt::Alignment vAlign;

    switch (index % 3) {
    case 0:
        hAlign = Qt::AlignLeft;
        break;
    case 1:
        hAlign = Qt::AlignHCenter;
        break;
    case 2:
        hAlign = Qt::AlignRight;
        break;
    }

    switch (index / 3) {
    case 0:
        vAlign = Qt::AlignTop;
        break;
    case 1:
        vAlign = Qt::AlignVCenter;
        break;
    case 2:
        vAlign = Qt::AlignBottom;
        break;
    }

    return hAlign | vAlign;
}

QRect AlignmentSelector::gridRect() const
{
    const int side = std::min(width(), height());
    int x = 0;
    int y = 0;

    if (alignmentMembers & Qt::AlignHCenter)
        x = (width() - side) / 2;
    else if (alignmentMembers & Qt::AlignRight)
        x = width() - side;

    if (alignmentMembers & Qt::AlignVCenter)
        y = (height() - side) / 2;
    else if (alignmentMembers & Qt::AlignBottom)
        y = height() - side;

    return QRect(x, y, side, side);
}

void AlignmentSelector::leaveEvent(QEvent *)
{
    hoveredCell = -1;
    update();
}

void AlignmentSelector::mouseMoveEvent(QMouseEvent *event)
{
    const QRect grid = gridRect();
    const int cellW = grid.width() / 3;
    const int cellH = grid.height() / 3;

    const QPoint pos = event->position().toPoint();
    if (!grid.contains(pos)) {
        if (hoveredCell != -1) {
            hoveredCell = -1;
            update();
        }
        return;
    }

    const int col = (pos.x() - grid.left()) / cellW;
    const int row = (pos.y() - grid.top()) / cellH;
    const int cell = row * 3 + col;

    if (hoveredCell != cell) {
        hoveredCell = cell;
        update();
    }
}

void AlignmentSelector::mousePressEvent(QMouseEvent *event)
{
    const QRect grid = gridRect();
    const int cellW = grid.width() / 3;
    const int cellH = grid.height() / 3;

    const QPoint pos = event->position().toPoint();
    if (!grid.contains(pos))
        return;

    const int col = (pos.x() - grid.left()) / cellW;
    const int row = (pos.y() - grid.top()) / cellH;

    selectCell(row * 3 + col);
}

void AlignmentSelector::keyPressEvent(QKeyEvent *event)
{
    int moveX = 0;
    int moveY = 0;

    switch (event->key()) {
    case Qt::Key_Left:
        moveX = -1;
        break;
    case Qt::Key_Right:
        moveX = 1;
        break;
    case Qt::Key_Up:
        moveY = -1;
        break;
    case Qt::Key_Down:
        moveY = 1;
        break;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        selectCell(focusedCell);
        return;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    moveFocusedCell(moveX, moveY);
}

void AlignmentSelector::moveFocusedCell(int moveX, int moveY)
{
    int row = focusedCell / 3;
    int col = focusedCell % 3;

    row = std::clamp(row + moveY, 0, 2);
    col = std::clamp(col + moveX, 0, 2);

    setFocusedCell(row * 3 + col);
}

void AlignmentSelector::setFocusedCell(int cell)
{
    if (cell != focusedCell) {
        focusedCell = cell;
        update();
    }
}

void AlignmentSelector::selectCell(int cell)
{
    setFocusedCell(cell);
    if (cell != selectedCell) {
        selectedCell = cell;
        emit alignmentChanged(cell);
    }
    update();
}

void AlignmentSelector::focusInEvent(QFocusEvent *)
{
    setFocusedCell(selectedCell);
    update();
}

void AlignmentSelector::focusOutEvent(QFocusEvent *)
{
    hoveredCell = -1;
    setFocusedCell(-1);
    update();
}
