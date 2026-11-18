#include "copy-paste.hpp"
#include "src/widgets/box.hpp"
#include "editor-model.hpp"
#include "editor-ui-helpers.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include <obs-module.h>
#include <obs-frontend-api.h>

extern "C" {
#include <obs.h>
}

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
    editorui::SetButtonIcon(
        btnCopy,
        editorui::GetIconPath("Copy.svg").c_str()
    );

    btnPasteRef = new QPushButton(box);
    btnPasteRef->setFixedHeight(26);
    btnPasteRef->setObjectName("pasteButton");
    btnPasteRef->setIconSize(QSize(18, 18));
    btnPasteRef->setText(obs_module_text("PasteReference"));
    editorui::SetButtonIcon(
        btnPasteRef,
        editorui::GetIconPath("Paste.svg").c_str()
    );

    btnPasteDup = new QPushButton(box);
    btnPasteDup->setFixedHeight(26);  
    btnPasteDup->setObjectName("pasteButton");
    btnPasteDup->setIconSize(QSize(18, 18));
    btnPasteDup->setText(obs_module_text("PasteDuplicate"));
    editorui::SetButtonIcon(
        btnPasteDup,
        editorui::GetIconPath("Paste.svg").c_str()
    );

    layout->addStretch();
    layout->addWidget(btnCopy);
    layout->addWidget(btnPasteRef);
    layout->addWidget(btnPasteDup);
    layout->addStretch();

    auto *boxLayout = qobject_cast<QVBoxLayout *>(box->layout());
    if (boxLayout) {
        boxLayout->addLayout(layout);
    }

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(box);

    connect(model, &EditorModel::sceneChanged, this, [this]() {
    });

    connect(btnCopy, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        if (editorui::TriggerNativeAction("actionCopySource"))
            return;

        obs_sceneitem_t *si = model->currentSceneItem();
        if (!si)
            return;

        obs_source_t *src = obs_sceneitem_get_source(si);
        if (!src)
            return;

        if (g_clipboardSource) {
            obs_source_release(g_clipboardSource);
            g_clipboardSource = nullptr;
        }

        obs_source_get_ref(src);
        g_clipboardSource = src;

        g_hasSourceClipboard = true;
    });

    connect(btnPasteRef, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        if (editorui::TriggerNativeAction("actionPasteRef"))
            return;

        if (!g_hasSourceClipboard || !g_clipboardSource)
            return;

        if (obs_source_is_group(g_clipboardSource))
            return;


        obs_scene_t *scene = model->currentScene();
        if (!scene)
            return;

        obs_source_get_ref(g_clipboardSource);

        obs_sceneitem_t *newItem =
            obs_scene_add(scene, g_clipboardSource);

        obs_source_release(g_clipboardSource);

        if (!newItem)
            return;

        model->setSelectedItem(newItem);
    });


    connect(btnPasteDup, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        if (editorui::TriggerNativeAction("actionPasteDup"))
            return;

        if (!g_clipboardSource)
            return;

        if (obs_source_get_output_flags(g_clipboardSource) &
            OBS_SOURCE_DO_NOT_DUPLICATE)
            return;

        obs_scene_t *scene = model->currentScene();
        if (!scene)
            return;

        const char *origName = obs_source_get_name(g_clipboardSource);
        QString baseName = QString::fromUtf8(origName);

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

                if (name == base) {
                    maxIdx = std::max(maxIdx, 2);
                    return true;
                }

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

        QString finalName = QString("%1 %2").arg(baseName).arg(maxIndex);
;


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

        obs_sceneitem_t *newItem = obs_scene_add(scene, dupSource);
        obs_source_release(dupSource);

        if (!newItem)
            return;

        model->setSelectedItem(newItem);

    });

    connect(btnCopy, &QPushButton::clicked,
            this, &CopyPaste::refreshStates);
}

bool CopyPaste::hasClipboard() const
{
    QAction *pasteRef = editorui::FindNativeAction("actionPasteRef");
    if (pasteRef && pasteRef->isEnabled())
        return true;
    QAction *pasteDup = editorui::FindNativeAction("actionPasteDup");
    if (pasteDup && pasteDup->isEnabled())
        return true;

    return g_hasSourceClipboard && g_clipboardSource != nullptr;
}

void CopyPaste::refreshStates()
{
    if (!model)
        return;

    const bool hasSelection = model->selectedItem() != nullptr;

    QAction *nativeRef = editorui::FindNativeAction("actionPasteRef");
    QAction *nativeDup = editorui::FindNativeAction("actionPasteDup");

    const bool nativeClip = (nativeRef && nativeRef->isEnabled()) ||
                            (nativeDup && nativeDup->isEnabled());

    bool refOk, dupOk;
    if (nativeClip) {
        refOk = nativeRef && nativeRef->isEnabled();
        dupOk = nativeDup && nativeDup->isEnabled();
    } else {
        const bool own = g_hasSourceClipboard && g_clipboardSource;
        refOk = own && !obs_source_is_group(g_clipboardSource);
        dupOk = own && !(obs_source_get_output_flags(g_clipboardSource) &
                         OBS_SOURCE_DO_NOT_DUPLICATE);
    }

    if (btnCopy)
        btnCopy->setEnabled(hasSelection);
    if (btnPasteRef)
        btnPasteRef->setEnabled(refOk);
    if (btnPasteDup)
        btnPasteDup->setEnabled(dupOk);
}
