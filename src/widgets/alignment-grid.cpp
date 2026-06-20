#include "alignment-grid.hpp"

#include <QGridLayout>

AlignmentGrid::AlignmentGrid(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);

    setFixedSize(33, 33); // 3 botones de 10px + 2 separaciones de 1px + 2 bordes de 1px

    for (int i = 0; i < 9; i++) {
        auto *btn = new QPushButton(this);
        btn->setCheckable(true);
        btn->setFixedSize(9, 9);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        btn->setObjectName("alignmentGridButton");

        btn->setStyleSheet(
            "QPushButton#alignmentGridButton { "
            "  border: 1px solid #808080; "
            "  border-radius: 2px; "
            "  background-color: transparent; "
            "} "
            "QPushButton#alignmentGridButton:checked { "
            "  background-color: palette(highlight); "
            "  border: 1px solid #808080; "
            "  border-radius: 2px; "
            "}"
        );

        buttons[i] = btn;

        int row = i / 3;
        int col = i % 3;
        grid->addWidget(btn, row, col);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            setSelected(i);
            emit alignmentChanged(i);
        });
    }
}

void AlignmentGrid::setSelected(int index)
{
    if (index < 0 || index > 8)
        return;

    currentIndex = index;
    updateSelection();
}

void AlignmentGrid::updateSelection()
{
    for (int i = 0; i < 9; i++)
        buttons[i]->setChecked(i == currentIndex);
}

/* ────────────────────────────────────────────────
   NUEVO — APAGAR / ENCENDER BOTONES DEL GRID
   ──────────────────────────────────────────────── */
void AlignmentGrid::setButtonsEnabled(bool enabled)
{
    for (auto *btn : buttons) {
        btn->setEnabled(enabled);
        btn->setStyleSheet(enabled
            ? "QPushButton#alignmentGridButton { border: 1px solid #808080; border-radius: 2px; background-color: transparent; }"
              "QPushButton#alignmentGridButton:checked { background-color: palette(highlight); border: 1px solid #808080; border-radius: 2px; }"
            : "QPushButton#alignmentGridButton { border: 1px solid #808080; border-radius: 2px; background-color: transparent; opacity: 0.4; }"
        );
    }
}
