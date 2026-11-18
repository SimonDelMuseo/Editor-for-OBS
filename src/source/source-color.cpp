#include "source-color.hpp"
#include "src/widgets/box.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSize>

#include <obs-module.h>

SourceColor::SourceColor(QWidget *parent)
    : QWidget(parent)
{
    auto *box = new Box(this);

    auto *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addStretch();

        static const char *presetColors[8] = {
        "rgba(255,0,0,0.33)",
        "rgba(255,255,0,0.33)",
        "rgba(0,255,0,0.33)",
        "rgba(0,255,255,0.33)",
        "rgba(0,0,255,0.33)",
        "rgba(255,0,255,0.33)",
        "rgba(0,0,0,0.33)"
        "rgba(255,255,255,0.33)",
        
    };


    for (int i = 0; i < 8; i++) {
        QPushButton *btn = new QPushButton(box);
        colorButtons[i] = btn;

        btn->setObjectName("colorPresetButton");
        btn->setFixedSize(26, 26);
        btn->setStyleSheet(QString(
            "QPushButton#colorPresetButton { "
            "background-color: %1; "
            "border: 1px solid black; "
            "border-radius: 4px; "
            "}").arg(presetColors[i]));

        layout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            emit presetSelected(i + 1);
        });
    }

    buttonSelect = new QPushButton(obs_module_text("CustomColor"), box);
    buttonSelect->setFixedHeight(26);
    layout->addWidget(buttonSelect);
    connect(buttonSelect, &QPushButton::clicked, this, [this]() {
        emit colorRequested();
    });

    buttonTrash = new QPushButton(box);
    buttonTrash->setFixedSize(26, 26);
    buttonTrash->setProperty("class", "icon-trash");
    layout->addWidget(buttonTrash);
    connect(buttonTrash, &QPushButton::clicked, this, [this]() {
        emit presetSelected(0);
    });

    layout->addStretch();

    auto *boxLayout = qobject_cast<QVBoxLayout *>(box->layout());
    if (boxLayout)
        boxLayout->addLayout(layout);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(box);

    activePresetIndex = 0;
    updatePresetButtonsVisual();
}

void SourceColor::updatePresetButtonsVisual()
{
    for (int i = 0; i < 8; i++) {
        QPushButton *btn = colorButtons[i];
        if (!btn)
            continue;

        if (activePresetIndex == i + 1)
            btn->setFixedSize(22, 22);
        else
            btn->setFixedSize(26, 26);
    }
}

void SourceColor::setCurrentPreset(int presetIndex)
{
    activePresetIndex = presetIndex;
    updatePresetButtonsVisual();
}

void SourceColor::setTrashEnabled(bool on)
{
    if (buttonTrash)
        buttonTrash->setEnabled(on);
}
