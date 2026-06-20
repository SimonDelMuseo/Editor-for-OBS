#pragma once

#include <QWidget>

class BoxFilters : public QWidget
{
    Q_OBJECT

public:
    explicit BoxFilters(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};
