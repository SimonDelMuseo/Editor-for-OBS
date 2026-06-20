#pragma once
#include <QWidget>
#include <QPushButton>
#include <array>

class AlignmentGrid : public QWidget
{
    Q_OBJECT
public:
    explicit AlignmentGrid(QWidget *parent = nullptr);

    void setSelected(int index);   // 0–8
    int  selected() const { return currentIndex; }

    /* NUEVO — permite apagar/encender el grid */
    void setButtonsEnabled(bool enabled);

signals:
    void alignmentChanged(int index);

private:
    std::array<QPushButton*, 9> buttons {};
    int currentIndex = -1;

    void updateSelection();
};
