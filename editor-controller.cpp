/* ────────────────────────────────────────────────
   INCLUDES
   ──────────────────────────────────────────────── */
#include "editor-controller.hpp"
#include "editor-dock.hpp"
#include "editor-model.hpp"

#include <QTimer>
#include <QList>

extern "C" {
#include <obs-frontend-api.h>
#include <obs.h>
}

/* ────────────────────────────────────────────────
   BÚSQUEDA RECURSIVA DEL ITEM SELECCIONADO
   ──────────────────────────────────────────────── */

struct FindSelectedContext {
    obs_sceneitem_t **found;
    QList<std::pair<obs_scene_t*, obs_sceneitem_t*>> *stack;
};

static bool find_selected_enum_cb(obs_scene_t *, obs_sceneitem_t *si, void *param)
{
    auto *ctx = static_cast<FindSelectedContext*>(param);

    if (obs_sceneitem_selected(si)) {
        *(ctx->found) = si;
        return false;
    }

    obs_source_t *src = obs_sceneitem_get_source(si);
    if (src) {
        const char *id = obs_source_get_unversioned_id(src);
        if (id && strcmp(id, "group") == 0) {
            obs_scene_t *sub = obs_group_from_source(src);
            if (sub)
                ctx->stack->append({ sub, si });
        }
    }

    return true;
}

static obs_sceneitem_t* find_selected_recursive(obs_scene_t *rootScene)
{
    if (!rootScene)
        return nullptr;

    QList<std::pair<obs_scene_t*, obs_sceneitem_t*>> stack;
    stack.append({ rootScene, nullptr });

    obs_sceneitem_t *found = nullptr;

    while (!stack.isEmpty() && !found) {
        auto pair = stack.takeLast();
        obs_scene_t *currentScene = pair.first;

        FindSelectedContext ctx;
        ctx.found = &found;
        ctx.stack = &stack;

        obs_scene_enum_items(currentScene, find_selected_enum_cb, &ctx);
    }

    return found;
}

/* ────────────────────────────────────────────────
   CALLBACK GLOBAL DE OBS
   ──────────────────────────────────────────────── */
void EditorController::frontend_event_callback(enum obs_frontend_event event,
                                               void *userdata)
{
    if (!userdata)
        return;

    auto *self = static_cast<EditorController*>(userdata);

    if (event == OBS_FRONTEND_EVENT_SCENE_CHANGED)
        self->handleSceneChanged();
}

/* ────────────────────────────────────────────────
   CONSTRUCTOR
   ──────────────────────────────────────────────── */
EditorController::EditorController(EditorDock *dock,
                                   EditorModel *model,
                                   QObject *parent)
    : QObject(parent)
    , dock(dock)
    , model(model)
{
    pollTimer = new QTimer(this);
    pollTimer->setInterval(150);

    connect(pollTimer, &QTimer::timeout,
            this, &EditorController::handleSelectionPoll);

    pollTimer->start();

    obs_frontend_add_event_callback(frontend_event_callback, this);

    obs_source_t *sceneSource = obs_frontend_get_current_scene();
    if (sceneSource) {
        obs_scene_t *scene = obs_scene_from_source(sceneSource);
        model->setCurrentScene(scene);
        obs_source_release(sceneSource);
    }
}

/* ────────────────────────────────────────────────
   DESTRUCTOR
   ──────────────────────────────────────────────── */
EditorController::~EditorController()
{
    obs_frontend_remove_event_callback(frontend_event_callback, this);
}

/* ────────────────────────────────────────────────
   MANEJO DE CAMBIO DE ESCENA
   ──────────────────────────────────────────────── */
void EditorController::handleSceneChanged()
{
    if (!model)
        return;

    obs_source_t *sceneSource = obs_frontend_get_current_scene();
    if (!sceneSource)
        return;

    obs_scene_t *scene = obs_scene_from_source(sceneSource);
    model->setCurrentScene(scene);

    obs_source_release(sceneSource);
}

/* ────────────────────────────────────────────────
   POLLING DE SELECCIÓN DE ITEM
   ──────────────────────────────────────────────── */
void EditorController::handleSelectionPoll()
{
    if (!dock || !model)
        return;

    obs_source_t *sceneSource = obs_frontend_get_current_scene();
    if (!sceneSource)
        return;

    obs_scene_t *scene = obs_scene_from_source(sceneSource);
    if (!scene) {
        obs_source_release(sceneSource);
        return;
    }

    obs_sceneitem_t *selected = find_selected_recursive(scene);

    model->setSelectedItem(selected);

    obs_source_release(sceneSource);
}
