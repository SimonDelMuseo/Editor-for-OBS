#include "copy-paste.hpp"
#include "src/widgets/box.hpp"
#include "editor-model.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include <obs-module.h>
#include <obs-frontend-api.h>

extern "C" {
#include <obs.h>
}

/* ────────────────────────────────────────────────
   CLIPBOARD GLOBAL — FUENTES
   ──────────────────────────────────────────────── */
static obs_source_t *g_clipboardSource = nullptr;
static bool g_hasSourceClipboard = false;

CopyPaste::CopyPaste(QWidget *parent)
    : QWidget(parent)
{
    auto *box = new Box(this);

    auto *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    btnCopy = new QPushButton(box);
    btnCopy->setFixedHeight(26);
    btnCopy->setObjectName("copyButton");
    btnCopy->setIconSize(QSize(18, 18));
    btnCopy->setText(obs_module_text("Copy"));
    //btnCopy->setStyleSheet("QPushButton { padding: 2 10px; text-align: center; }");

    btnPasteRef = new QPushButton(box);
    btnPasteRef->setFixedHeight(26);
    btnPasteRef->setObjectName("pasteButton");
    btnPasteRef->setIconSize(QSize(18, 18));
    btnPasteRef->setText(obs_module_text("PasteReference"));
    //btnPasteRef->setStyleSheet("QPushButton { padding: 2 10px; text-align: center; }");

    btnPasteDup = new QPushButton(box);
    btnPasteDup->setFixedHeight(26);  
    btnPasteDup->setObjectName("pasteButton");
    btnPasteDup->setIconSize(QSize(18, 18));
    btnPasteDup->setText(obs_module_text("PasteDuplicate"));
    //btnPasteDup->setStyleSheet("QPushButton { padding: 2 10px; text-align: center; }");

    layout->addStretch();
    layout->addWidget(btnCopy);
    layout->addWidget(btnPasteRef);
    layout->addWidget(btnPasteDup);
    layout->addStretch();

    auto *boxLayout = qobject_cast<QVBoxLayout *>(box->layout());
    if (boxLayout) {
        boxLayout->addLayout(layout);
        //boxLayout->setAlignment(layout, Qt::AlignVCenter);
    }

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(box);

     /* ────────────────────────────────────────────────
       SINCRONIZAR CON CAMBIO DE ESCENA
       ──────────────────────────────────────────────── */
    connect(model, &EditorModel::sceneChanged, this, [this]() {
        // No hace falta hacer nada, solo recibir el evento
    });

    /* ────────────────────────────────────────────────
       COPIAR 
       ──────────────────────────────────────────────── */
    connect(btnCopy, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        // Usamos directamente las variables internas del modelo
        obs_sceneitem_t *si = model->currentSceneItem();
        if (!si)
            return;

        obs_source_t *src = obs_sceneitem_get_source(si);
        if (!src)
            return;

        // Limpiar portapapeles anterior
        if (g_clipboardSource) {
            obs_source_release(g_clipboardSource);
            g_clipboardSource = nullptr;
        }

        // Guardar nueva referencia
        obs_source_get_ref(src);
        g_clipboardSource = src;

        g_hasSourceClipboard = true;
    });

    /* ────────────────────────────────────────────────
    PEGAR REFERENCIA 
    ──────────────────────────────────────────────── */
    connect(btnPasteRef, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        if (!g_hasSourceClipboard || !g_clipboardSource)
            return;

        // Item seleccionado actual
        //obs_sceneitem_t *si = model->currentSceneItem();
        //if (!si)
           //return;

        // Escena actual
        obs_scene_t *scene = model->currentScene();
        if (!scene)
            return;

        // ⭐ NECESARIO PARA PERMITIR PEGAR REFERENCIA EN OTRA ESCENA
        obs_source_get_ref(g_clipboardSource);

        // Pegar referencia
        obs_sceneitem_t *newItem =
            obs_scene_add(scene, g_clipboardSource);

        // Liberar ref extra
        obs_source_release(g_clipboardSource);

        if (!newItem)
            return;

        // Seleccionar el nuevo item
        model->setSelectedItem(newItem);
    });


    /* ────────────────────────────────────────────────
    PEGAR DUPLICADO 
    ──────────────────────────────────────────────── */
    connect(btnPasteDup, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        if (!g_clipboardSource)
            return;

        // 1) Obtener escena actual desde el modelo
        obs_scene_t *scene = model->currentScene();
        if (!scene)
            return;

        // 2) Nombre base del original
        const char *origName = obs_source_get_name(g_clipboardSource);
        QString baseName = QString::fromUtf8(origName);

        // 3) Buscar nombres existentes en la escena
        int maxIndex = 1;

        QPair<QString,int> nameData(baseName, maxIndex);

        obs_scene_enum_items(
            scene,
            [](obs_scene_t *, obs_sceneitem_t *si, void *param) -> bool {

                auto *data = static_cast<QPair<QString,int>*>(param);
                QString base = data->first;
                int &maxIdx = data->second;

                obs_source_t *src = obs_sceneitem_get_source(si);
                if (!src)
                    return true;

                QString name = QString::fromUtf8(obs_source_get_name(src));

                // Coincide exactamente → "Copiloto"
                if (name == base) {
                    maxIdx = std::max(maxIdx, 2);
                    return true;
                }

                // Coincide con patrón "Copiloto X"
                if (name.startsWith(base + " ")) {
                    bool ok = false;
                    int num = name.mid(base.length() + 1).toInt(&ok);
                    if (ok)
                        maxIdx = std::max(maxIdx, num + 1);
                }

                return true;
            },
            &nameData
        );

        maxIndex = nameData.second;

        // 4) Nombre final único
        QString finalName = QString("%1 %2").arg(baseName).arg(maxIndex);
;

        // 5) Crear duplicado con nombre único a partir de settings

        const char *source_id = obs_source_get_id(g_clipboardSource);
        obs_data_t *settings = obs_source_get_settings(g_clipboardSource);
        obs_data_addref(settings);

        obs_source_t *dupSource =
            obs_source_create(source_id,
                            finalName.toUtf8().constData(),
                            settings,
                            nullptr);

        obs_data_release(settings);

        if (!dupSource)
            return;

        // 6) Añadir a la escena
        obs_sceneitem_t *newItem = obs_scene_add(scene, dupSource);
        obs_source_release(dupSource);

        if (!newItem)
            return;

        // 7) Seleccionar el nuevo item
        model->setSelectedItem(newItem);

    });

}
