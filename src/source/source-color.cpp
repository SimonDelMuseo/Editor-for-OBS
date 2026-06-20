#include "source-color.hpp"
#include "src/widgets/box.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSize>

#include <obs-module.h>

/* ────────────────────────────────────────────────
   CONSTRUCTOR
   ──────────────────────────────────────────────── */
SourceColor::SourceColor(QWidget *parent)
    : QWidget(parent)
{
    auto *box = new Box(this);

    auto *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addStretch();

    /* Colores CSS de OBS */
        static const char *presetColors[8] = {
        "rgba(255,0,0,0.33)",     // rojo
        "rgba(255,255,0,0.33)",   // amarillo
        "rgba(0,255,0,0.33)",     // verde
        "rgba(0,255,255,0.33)",   // cian
        "rgba(0,0,255,0.33)",     // azul
        "rgba(255,0,255,0.33)",   // magenta
        "rgba(0,0,0,0.33)"        // negro
        "rgba(255,255,255,0.33)", // gris
        
    };


    /* Crear botones de presets */
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

        /* Emitir presetSelected(i+1) */
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            emit presetSelected(i + 1);
        });
    }

    /* Botón Custom Color */
    buttonSelect = new QPushButton(obs_module_text("CustomColor"), box);
    buttonSelect->setFixedHeight(26);
    layout->addWidget(buttonSelect);
    connect(buttonSelect, &QPushButton::clicked, this, [this]() {
        emit colorRequested();
    });

    /* Botón borrar color */
    buttonTrash = new QPushButton(box);
    buttonTrash->setFixedSize(26, 26);
    buttonTrash->setProperty("class", "icon-trash");
    layout->addWidget(buttonTrash);
    connect(buttonTrash, &QPushButton::clicked, this, [this]() {
        emit presetSelected(0);   // ← borrar color
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

/* ────────────────────────────────────────────────
   ACTUALIZAR VISUAL
   ──────────────────────────────────────────────── */
void SourceColor::updatePresetButtonsVisual()
{
    for (int i = 0; i < 8; i++) {
        QPushButton *btn = colorButtons[i];
        if (!btn)
            continue;

        if (activePresetIndex == i + 1)
            btn->setFixedSize(22, 22);  // activo
        else
            btn->setFixedSize(26, 26);  // normal
    }
}

/* ────────────────────────────────────────────────
   SINCRONIZAR PRESET ACTUAL
   ──────────────────────────────────────────────── */
void SourceColor::setCurrentPreset(int presetIndex)
{
    activePresetIndex = presetIndex;  // 0..8
    updatePresetButtonsVisual();
}
