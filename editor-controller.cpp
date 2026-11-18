#include "editor-controller.hpp"
#include "editor-dock.hpp"
#include "editor-model.hpp"

#include <QTimer>
#include <QList>

extern "C" {
#include <obs-frontend-api.h>
#include <obs.h>
}


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

void EditorController::frontend_event_callback(enum obs_frontend_event event,
                                               void *userdata)
{
    if (!userdata)
        return;

    auto *self = static_cast<EditorController*>(userdata);

    switch (event) {
    case OBS_FRONTEND_EVENT_SCENE_CHANGED:
    case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
    case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
    case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
    case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
    case OBS_FRONTEND_EVENT_FINISHED_LOADING:
        self->handleSceneChanged();
        break;

    case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
        self->handleSceneChanged();
        if (self->model)
            self->model->notifySceneListChanged();
        break;

    case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
        self->disconnectSceneSignals();
        if (self->model)
            self->model->invalidate();
        break;

    case OBS_FRONTEND_EVENT_EXIT:
        if (self->pollTimer)
            self->pollTimer->stop();
        self->disconnectSceneSignals();
        self->disconnectGlobalSignals();
        if (self->model)
            self->model->invalidate();
        break;

    default:
        break;
    }
}

EditorController::EditorController(EditorDock *dock,
                                   EditorModel *model,
                                   QObject *parent)
    : QObject(parent)
    , dock(dock)
    , model(model)
{
    pollTimer = new QTimer(this);
    pollTimer->setInterval(2000);

    connect(pollTimer, &QTimer::timeout,
            this, &EditorController::handleSafetyPoll);

    pollTimer->start();

    obs_frontend_add_event_callback(frontend_event_callback, this);

    connectGlobalSignals();
    handleSceneChanged();
}

EditorController::~EditorController()
{
    obs_frontend_remove_event_callback(frontend_event_callback, this);
    disconnectSceneSignals();
    disconnectGlobalSignals();
}

void EditorController::handleSceneChanged()
{
    if (!model)
        return;

    obs_source_t *sceneSource = acquireActiveScene();
    if (!sceneSource)
        return;

    obs_scene_t *scene = obs_scene_from_source(sceneSource);
    model->setCurrentScene(scene);

    obs_source_release(sceneSource);

    connectSceneSignals();

    rescanSelection();
}

void EditorController::handleSafetyPoll()
{
    rescanSelection();
}

obs_source_t *EditorController::acquireActiveScene() const
{
    obs_source_t *preview = obs_frontend_get_current_preview_scene();
    if (preview)
        return preview;

    return obs_frontend_get_current_scene();
}

void EditorController::rescanSelection()
{
    if (!dock || !model)
        return;

    obs_source_t *sceneSource = acquireActiveScene();
    if (!sceneSource) {
        model->syncSelection(nullptr);
        return;
    }

    obs_scene_t *scene = obs_scene_from_source(sceneSource);

    if (scene != model->currentScene()) {
        model->setCurrentScene(scene);
        obs_source_release(sceneSource);
        connectSceneSignals();
        rescanSelection();
        return;
    }

    obs_sceneitem_t *selected = find_selected_recursive(scene);

    model->syncSelection(selected);

    obs_source_release(sceneSource);
}

void EditorController::connectSceneSignals()
{
    disconnectSceneSignals();

    if (!model || !model->currentScene())
        return;

    obs_source_t *src = obs_scene_get_source(model->currentScene());
    if (!src)
        return;

    boundSceneSource = obs_source_get_ref(src);
    if (!boundSceneSource)
        return;

    signal_handler_t *sh = obs_source_get_signal_handler(boundSceneSource);
    if (!sh) {
        obs_source_release(boundSceneSource);
        boundSceneSource = nullptr;
        return;
    }

    signal_handler_connect(sh, "item_select",    scene_item_select_cb,   this);
    signal_handler_connect(sh, "item_deselect",  scene_item_deselect_cb, this);
    signal_handler_connect(sh, "item_remove",    scene_item_remove_cb,   this);
    signal_handler_connect(sh, "item_add",       scene_item_add_cb,      this);
    signal_handler_connect(sh, "item_transform", scene_item_transform_cb, this);
    signal_handler_connect(sh, "item_visible",   scene_item_state_cb,    this);
    signal_handler_connect(sh, "item_locked",    scene_item_state_cb,    this);
}

void EditorController::disconnectSceneSignals()
{
    if (!boundSceneSource)
        return;

    signal_handler_t *sh = obs_source_get_signal_handler(boundSceneSource);
    if (sh) {
        signal_handler_disconnect(sh, "item_select",    scene_item_select_cb,   this);
        signal_handler_disconnect(sh, "item_deselect",  scene_item_deselect_cb, this);
        signal_handler_disconnect(sh, "item_remove",    scene_item_remove_cb,   this);
        signal_handler_disconnect(sh, "item_add",       scene_item_add_cb,      this);
        signal_handler_disconnect(sh, "item_transform", scene_item_transform_cb, this);
        signal_handler_disconnect(sh, "item_visible",   scene_item_state_cb,    this);
        signal_handler_disconnect(sh, "item_locked",    scene_item_state_cb,    this);
    }

    obs_source_release(boundSceneSource);
    boundSceneSource = nullptr;
}

void EditorController::connectGlobalSignals()
{
    signal_handler_t *global = obs_get_signal_handler();
    if (!global)
        return;

    signal_handler_connect(global, "source_filter_add", global_filter_changed_cb, this);
    signal_handler_connect(global, "source_filter_remove", global_filter_changed_cb, this);
}

void EditorController::disconnectGlobalSignals()
{
    signal_handler_t *global = obs_get_signal_handler();
    if (!global)
        return;

    signal_handler_disconnect(global, "source_filter_add", global_filter_changed_cb, this);
    signal_handler_disconnect(global, "source_filter_remove", global_filter_changed_cb, this);
}

void EditorController::scene_item_select_cb(void *data, calldata_t *)
{
    auto *self = static_cast<EditorController *>(data);
    if (self)
        QMetaObject::invokeMethod(self, [self]() { self->rescanSelection(); },
                                  Qt::QueuedConnection);
}

void EditorController::scene_item_deselect_cb(void *data, calldata_t *)
{
    auto *self = static_cast<EditorController *>(data);
    if (self)
        QMetaObject::invokeMethod(self, [self]() { self->rescanSelection(); },
                                  Qt::QueuedConnection);
}

void EditorController::scene_item_remove_cb(void *data, calldata_t *)
{
    auto *self = static_cast<EditorController *>(data);
    if (self)
        QMetaObject::invokeMethod(self, [self]() {
            if (self->model) {
                self->model->invalidateUsedInCache();
                self->model->notifySceneItemsChanged();
            }
            self->rescanSelection();
        }, Qt::QueuedConnection);
}

void EditorController::scene_item_add_cb(void *data, calldata_t *)
{
    auto *self = static_cast<EditorController *>(data);
    if (self)
        QMetaObject::invokeMethod(self, [self]() {
            if (self->model) {
                self->model->invalidateUsedInCache();
                self->model->notifySceneItemsChanged();
            }
        }, Qt::QueuedConnection);
}
void EditorController::scene_item_transform_cb(void *data, calldata_t *)
{
    auto *self = static_cast<EditorController *>(data);
    if (self)
        QMetaObject::invokeMethod(self, [self]() {
            if (self->model)
                self->model->notifyTransformChanged();
        }, Qt::QueuedConnection);
}

void EditorController::scene_item_state_cb(void *data, calldata_t *)
{
    auto *self = static_cast<EditorController *>(data);
    if (self)
        QMetaObject::invokeMethod(self, [self]() {
            if (self->model) {
                self->model->refreshItemCaches();
                emit self->model->modelUpdated();
            }
        }, Qt::QueuedConnection);
}


void EditorController::global_filter_changed_cb(void *data, calldata_t *)
{
    auto *self = static_cast<EditorController *>(data);
    if (self)
        QMetaObject::invokeMethod(self, [self]() {
            if (self->model)
                self->model->notifyFiltersChanged();
        }, Qt::QueuedConnection);
}