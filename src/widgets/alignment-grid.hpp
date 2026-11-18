#pragma once

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWidget>


class AlignmentSelector : public QWidget
{
    Q_OBJECT

public:
    explicit AlignmentSelector(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setSelected(int index);
    void setButtonsEnabled(bool enabled);

    int currentIndex() const;
    Qt::Alignment value() const;

signals:
    void alignmentChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    int hoveredCell  = -1;
    int focusedCell  = 4;
    int selectedCell = 4;

    Qt::Alignment alignmentMembers = Qt::AlignTop | Qt::AlignLeft;

    QRect gridRect() const;
    QRect cellRect(int index) const;
    Qt::Alignment cellAlignment(int index) const;
    void moveFocusedCell(int moveX, int moveY);
    void setFocusedCell(int cell);
    void selectCell(int cell);
};
