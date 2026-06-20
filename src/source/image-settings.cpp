#include "src/source/image-settings.hpp"
#include "editor-model.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QMenu>
#include <QGuiApplication>
#include <QScreen>

#include <obs-frontend-api.h>
#include <obs-module.h>

/* ────────────────────────────────────────────────
   IMAGE SETTINGS — UI ELEMENTS (NUEVO DISEÑO)
   ──────────────────────────────────────────────── */
ImageSettings::ImageSettings(QWidget *parent)
    : Box(parent)
{
    auto *layout = qobject_cast<QVBoxLayout *>(this->layout());
    layout->setSpacing(6);

    /* ────────────────────────────────────────────────
       LÍNEA 1:
       Save Source Screenshot + Open Source Projector
       ──────────────────────────────────────────────── */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(6);

        row->addStretch();

        /* Botón de screenshot */
        saveScreenshotButton = new QPushButton(obs_module_text("SaveSourceScreenshot"));
        saveScreenshotButton->setObjectName("SaveSourceScreenshot");
        saveScreenshotButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        row->addWidget(saveScreenshotButton);

        /* 🔥 LÓGICA DEL BOTÓN — EXACTAMENTE COMO OBS */
        connect(saveScreenshotButton, &QPushButton::clicked, this, [this]() {
            if (!model)
                return;

            obs_source_t *src = model->selectedSource();
            if (!src)
                return;

            obs_frontend_take_source_screenshot(src);
        });

        row->addSpacing(10);

        /* Botón que abrirá el menú de proyectores */
        projectorMenuButton = new QPushButton(obs_module_text("OpenSourceProjector"));
        projectorMenuButton->setObjectName("OpenSourceProjector");
        projectorMenuButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        projectorMenu = new QMenu(this);
        projectorMenuButton->setMenu(projectorMenu);

        /* Rellenar menú dinámico de proyectores */
        projectorMenu->clear();

        QList<QScreen*> screens = QGuiApplication::screens();

        for (int i = 0; i < screens.size(); i++) {
            QString name = screens[i]->name();
            QAction *act = projectorMenu->addAction(name);

            connect(act, &QAction::triggered, this, [this, i]() {
                emit openProjectorOnMonitorRequested(i);
            });
        }

        projectorMenu->addSeparator();

        QAction *newProj = projectorMenu->addAction(obs_module_text("ProjectorNew"));
        connect(newProj, &QAction::triggered, this, [this]() {
            emit openProjectorWindowRequested();
        });

        row->addWidget(projectorMenuButton);

        row->addStretch();

        layout->addLayout(row);
    }

    /* ────────────────────────────────────────────────
       LÍNEA 2:
       Tres cajitas horizontales (texto arriba, combo abajo)
       ──────────────────────────────────────────────── */
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(20);

        row->addStretch();

        /* ──────────────── CAJA 1 — Scale Filtering ──────────────── */
        {
            auto *box = new QVBoxLayout();
            box->setSpacing(4);
            box->setAlignment(Qt::AlignHCenter);

            auto *label = new QLabel(obs_module_text("ScaleFiltering"));
            label->setObjectName("ScaleFiltering");
            label->setStyleSheet("font-weight: bold;");
            label->setAlignment(Qt::AlignCenter);
            box->addWidget(label);

            scaleFilteringCombo = new QComboBox();
            scaleFilteringCombo->setObjectName("ScaleFiltering");
            scaleFilteringCombo->setMinimumWidth(120);

            scaleFilteringCombo->addItem(obs_module_text("ScaleFiltering_Disable"));
            scaleFilteringCombo->addItem(obs_module_text("ScaleFiltering_Point"));
            scaleFilteringCombo->addItem(obs_module_text("ScaleFiltering_Bilinear"));
            scaleFilteringCombo->addItem(obs_module_text("ScaleFiltering_Bicubic"));
            scaleFilteringCombo->addItem(obs_module_text("ScaleFiltering_Lanczos"));
            scaleFilteringCombo->addItem(obs_module_text("ScaleFiltering_Area"));

            box->addWidget(scaleFilteringCombo);

            connect(scaleFilteringCombo, &QComboBox::currentIndexChanged, this,
                    [this](int index) {
                        QString key;
                        switch (index) {
                        case 0: key = "disable"; break;
                        case 1: key = "point"; break;
                        case 2: key = "bilinear"; break;
                        case 3: key = "bicubic"; break;
                        case 4: key = "lanczos"; break;
                        case 5: key = "area"; break;
                        default: key = "disable"; break;
                        }

                        emit scaleFilteringChanged(key);
                    });

            row->addLayout(box);
        }

        /* ──────────────── CAJA 2 — Blending Mode ──────────────── */
        {
            auto *box = new QVBoxLayout();
            box->setSpacing(4);
            box->setAlignment(Qt::AlignHCenter);

            auto *label = new QLabel(obs_module_text("BlendingMode"));
            label->setObjectName("BlendingMode");
            label->setStyleSheet("font-weight: bold;");
            label->setAlignment(Qt::AlignCenter);
            box->addWidget(label);

            blendingModeCombo = new QComboBox();
            blendingModeCombo->setObjectName("BlendingMode");
            blendingModeCombo->setMinimumWidth(120);

            blendingModeCombo->addItem(obs_module_text("BlendingMode_Normal"));
            blendingModeCombo->addItem(obs_module_text("BlendingMode_Add"));
            blendingModeCombo->addItem(obs_module_text("BlendingMode_Subtract"));
            blendingModeCombo->addItem(obs_module_text("BlendingMode_Screen"));
            blendingModeCombo->addItem(obs_module_text("BlendingMode_Multiply"));
            blendingModeCombo->addItem(obs_module_text("BlendingMode_Lighten"));
            blendingModeCombo->addItem(obs_module_text("BlendingMode_Darken"));

            box->addWidget(blendingModeCombo);
            row->addLayout(box);
        }

        /* ──────────────── CAJA 3 — Blending Method ──────────────── */
        {
            auto *box = new QVBoxLayout();
            box->setSpacing(4);
            box->setAlignment(Qt::AlignHCenter);

            auto *label = new QLabel(obs_module_text("BlendingMethod"));
            label->setObjectName("BlendingMethod");
            label->setStyleSheet("font-weight: bold;");
            label->setAlignment(Qt::AlignCenter);
            box->addWidget(label);

            blendingMethodCombo = new QComboBox();
            blendingMethodCombo->setObjectName("BlendingMethod");
            blendingMethodCombo->setMinimumWidth(120);

            blendingMethodCombo->addItem(obs_module_text("BlendingMethod_Default"));
            blendingMethodCombo->addItem(obs_module_text("BlendingMethod_SRGBOff"));

            box->addWidget(blendingMethodCombo);
            row->addLayout(box);
        }

        row->addStretch();

        layout->addLayout(row);
    }
}

/* ────────────────────────────────────────────────
   SET SCALE FILTERING (UI UPDATE)
   ──────────────────────────────────────────────── */
void ImageSettings::setScaleFiltering(const QString &value)
{
    if (!scaleFilteringCombo)
        return;

    int index = 0;

    if (value == "disable")       index = 0;
    else if (value == "point")    index = 1;
    else if (value == "bilinear") index = 2;
    else if (value == "bicubic")  index = 3;
    else if (value == "lanczos")  index = 4;
    else if (value == "area")     index = 5;

    QSignalBlocker blocker(scaleFilteringCombo);
    scaleFilteringCombo->setCurrentIndex(index);
}
