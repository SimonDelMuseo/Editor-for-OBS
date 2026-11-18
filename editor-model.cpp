#include "editor-model.hpp"
#include "editor-ui-helpers.hpp"
#include "obs-module.h"
#include <obs.h>
#include <obs-data.h>
#include <QColor>
#include <QList>
#include <QMainWindow>
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QStyle>
#include <cstring>
#include <graphics/vec3.h>
#include <graphics/vec4.h>
#include <graphics/matrix4.h>

extern "C" {
#include <obs-frontend-api.h>
}

struct UsedInData {
    obs_source_t *target = nullptr;
    int count = 0;
};

struct ItemSearchData {
    obs_source_t *target = nullptr;
    bool found = false;
};

static bool search_in_item(obs_sceneitem_t *item, obs_source_t *target);

static bool enum_group_items_callback(obs_scene_t *scene,
                                      obs_sceneitem_t *child,
                                      void *param)
{
    Q_UNUSED(scene);

    auto *data = reinterpret_cast<ItemSearchData *>(param);
    if (!data || data->found)
        return false;

    if (search_in_item(child, data->target)) {
        data->found = true;
        return false;
    }

    return true;
}

static bool search_in_item(obs_sceneitem_t *item, obs_source_t *target)
{
    if (!item || !target)
        return false;

    obs_source_t *itemSource = obs_sceneitem_get_source(item);
    if (!itemSource)
        return false;

    if (itemSource == target)
        return true;

    if (obs_source_is_group(itemSource)) {
        obs_scene_t *subScene = obs_group_from_source(itemSource);
        if (!subScene)
            return false;

        ItemSearchData data;
        data.target = target;
        data.found  = false;

        obs_scene_enum_items(subScene, enum_group_items_callback, &data);
        return data.found;
    }

    return false;
}

static bool enum_scene_items_callback(obs_scene_t *scene,
                                      obs_sceneitem_t *item,
                                      void *param)
{
    Q_UNUSED(scene);

    auto *data = reinterpret_cast<ItemSearchData *>(param);
    if (!data || data->found)
        return false;

    if (search_in_item(item, data->target)) {
        data->found = true;
        return false;
    }

    return true;
}

static bool enum_scenes_callback(void *param, obs_source_t *sceneSource)
{
    auto *data = reinterpret_cast<UsedInData *>(param);
    if (!data || !data->target || !sceneSource)
        return true;

    obs_scene_t *scene = obs_scene_from_source(sceneSource);
    if (!scene)
        return true;

    ItemSearchData search;
    search.target = data->target;
    search.found  = false;

    obs_scene_enum_items(scene, enum_scene_items_callback, &search);

    if (search.found)
        data->count++;

    return true;
}

EditorModel::EditorModel(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
}

EditorModel::~EditorModel()
{
    releaseItemRefs();
    releaseSceneRefs();

    if (clipboardTransition) {
        obs_data_release(clipboardTransition);
        clipboardTransition = nullptr;
    }

    if (s_instance == this)
        s_instance = nullptr;
}

EditorModel *EditorModel::s_instance = nullptr;

void EditorModel::releaseItemRefs()
{
    invalidateUsedInCache();

    disconnectSourceSignals();

    if (currentItem) {
        obs_sceneitem_release(currentItem);
        currentItem = nullptr;
    }
    if (currentSource) {
        obs_source_release(currentSource);
        currentSource = nullptr;
    }
}

void EditorModel::releaseSceneRefs()
{
    sceneSource = nullptr;
    if (scene) {
        obs_scene_release(scene);
        scene = nullptr;
    }
}

void EditorModel::invalidate()
{
    releaseItemRefs();
    releaseSceneRefs();

    sourceNameCache = "-";
    sourceTypeCache = "-";
    visibleCache = false;
    lockedCache  = false;
    sceneNameCache.clear();
    invalidateUsedInCache();

    emit sceneChanged();
    emit modelUpdated();
}

void EditorModel::setSelectedItem(obs_sceneitem_t *item)
{
    obs_source_t *newBorrowed = item ? obs_sceneitem_get_source(item) : nullptr;
    obs_source_t *newSource   = newBorrowed ? obs_source_get_ref(newBorrowed) : nullptr;

    if (item)
        obs_sceneitem_addref(item);

    releaseItemRefs();

    currentItem   = item;
    currentSource = newSource;

    connectSourceSignals();

    updateItemData();
    updateSceneData();

    emit modelUpdated();
}

void EditorModel::sourceStateCb(void *param, calldata_t *)
{
    auto *m = static_cast<EditorModel *>(param);
    if (m)
        QMetaObject::invokeMethod(m, [m]() { emit m->modelUpdated(); },
                                  Qt::QueuedConnection);
}

void EditorModel::sourceDestroyCb(void *param, calldata_t *)
{
    auto *m = static_cast<EditorModel *>(param);
    if (!m)
        return;
    m->disconnectSourceSignals();
    QMetaObject::invokeMethod(m, [m]() { emit m->modelUpdated(); },
                              Qt::QueuedConnection);
}

void EditorModel::connectSourceSignals()
{
    if (!currentSource || connectedSource == currentSource)
        return;

    disconnectSourceSignals();

    signal_handler_t *sh = obs_source_get_signal_handler(currentSource);
    if (!sh)
        return;

    signal_handler_connect(sh, "activate",         sourceStateCb, this);
    signal_handler_connect(sh, "deactivate",       sourceStateCb, this);
    signal_handler_connect(sh, "audio_activate",   sourceStateCb, this);
    signal_handler_connect(sh, "audio_deactivate", sourceStateCb, this);
    signal_handler_connect(sh, "audio_mixers",     sourceStateCb, this);
    signal_handler_connect(sh, "destroy",          sourceDestroyCb, this);
    connectedSource = currentSource;
}

void EditorModel::disconnectSourceSignals()
{
    if (!connectedSource)
        return;

    signal_handler_t *sh = obs_source_get_signal_handler(connectedSource);
    if (sh) {
        signal_handler_disconnect(sh, "activate",         sourceStateCb, this);
        signal_handler_disconnect(sh, "deactivate",       sourceStateCb, this);
        signal_handler_disconnect(sh, "audio_activate",   sourceStateCb, this);
        signal_handler_disconnect(sh, "audio_deactivate", sourceStateCb, this);
        signal_handler_disconnect(sh, "audio_mixers",     sourceStateCb, this);
        signal_handler_disconnect(sh, "destroy",          sourceDestroyCb, this);
    }
    connectedSource = nullptr;
}

void EditorModel::syncSelection(obs_sceneitem_t *item)
{
    if (item != currentItem) {
        setSelectedItem(item);
        return;
    }

    const QString prevName = sourceNameCache;
    const bool    prevVis  = visibleCache;
    const bool    prevLock = lockedCache;

    updateItemData();

    if (prevName != sourceNameCache || prevVis != visibleCache ||
        prevLock != lockedCache)
        emit modelUpdated();
}

void EditorModel::refreshItemCaches()
{
    updateItemData();
    emit modelUpdated();
}

void EditorModel::refreshSceneCaches()
{
    updateSceneData();
    emit sceneChanged();
}

void EditorModel::notifySceneListChanged()
{
    invalidateUsedInCache();
    emit sceneListChanged();
}

void EditorModel::notifySceneItemsChanged()
{
    emit sceneItemsChanged();
}

void EditorModel::notifyTransformChanged()
{
    emit transformChanged();
}

void EditorModel::notifyFiltersChanged()
{
    emit filtersChangedExternally();
}


void EditorModel::invalidateUsedInCache()
{
    usedInValid  = false;
    usedInSource = nullptr;
    usedInCount  = 0;
}

void EditorModel::setCurrentScene(obs_scene_t *newScene)
{
    obs_scene_t *newRef = newScene ? obs_scene_get_ref(newScene) : nullptr;

    releaseItemRefs();
    releaseSceneRefs();

    scene       = newRef;
    sceneSource = scene ? obs_scene_get_source(scene) : nullptr;

    updateSceneData();
    updateItemData();
    invalidateUsedInCache();

    emit sceneChanged();
    emit modelUpdated();
}

void EditorModel::updateItemData()
{
    if (!currentSource) {
        sourceNameCache = "-";
        sourceTypeCache = "-";
        visibleCache = false;
        lockedCache  = false;
        return;
    }

    const char *name = obs_source_get_name(currentSource);
    sourceNameCache = name ? QString::fromUtf8(name) : QString();

    const char *id = obs_source_get_unversioned_id(currentSource);
    sourceTypeCache = id ? QString::fromUtf8(id) : QString();

    visibleCache = obs_sceneitem_visible(currentItem);
    lockedCache  = obs_sceneitem_locked(currentItem);
}

void EditorModel::updateSceneData()
{
    if (!sceneSource) {
        sceneNameCache.clear();
        return;
    }

    const char *name = obs_source_get_name(sceneSource);
    sceneNameCache = name ? QString::fromUtf8(name) : QString();
}

QString EditorModel::sourceSize() const
{
    if (!currentSource)
        return "-";

    uint32_t w = obs_source_get_width(currentSource);
    uint32_t h = obs_source_get_height(currentSource);

    if (w == 0 || h == 0)
        return "-";

    return QString("%1 x %2").arg(w).arg(h);
}

QString EditorModel::scaledSize() const
{
    if (!currentItem || !currentSource)
        return "-";

    uint32_t w = obs_source_get_width(currentSource);
    uint32_t h = obs_source_get_height(currentSource);

    if (w == 0 || h == 0)
        return "-";

    obs_transform_info oti;
    obs_sceneitem_get_info2(currentItem, &oti);

    if (oti.bounds_type != OBS_BOUNDS_NONE) {

        if (oti.crop_to_bounds) {
            return QString("%1 x %2")
                .arg(int(oti.bounds.x))
                .arg(int(oti.bounds.y));
        }

        return QString("%1 x %2")
            .arg(int(oti.bounds.x))
            .arg(int(oti.bounds.y));
    }

    vec2 scale;
    obs_sceneitem_get_scale(currentItem, &scale);

    int scaledW = int(w * scale.x);
    int scaledH = int(h * scale.y);

    return QString("%1 x %2").arg(scaledW).arg(scaledH);
}

QString EditorModel::usedInScenes() const
{
    if (!currentSource)
        return "-";

    if (!usedInValid || usedInSource != currentSource) {
        UsedInData data;
        data.target = currentSource;
        data.count  = 0;

        obs_enum_scenes(enum_scenes_callback, &data);

        usedInValid  = true;
        usedInSource = currentSource;
        usedInCount  = data.count;
    }

    QString word = (usedInCount == 1)
    ? QString(obs_module_text("SceneSingular"))
    : QString(obs_module_text("ScenePlural"));

    return QString("%1 %2").arg(usedInCount).arg(word);
}

int EditorModel::currentItemPreset() const
{
    if (!currentItem)
        return 0;

    obs_data_t *priv = obs_sceneitem_get_private_settings(currentItem);
    if (!priv)
        return 0;

    const char *colorStr = obs_data_get_string(priv, "color");
    if (colorStr && *colorStr != 0) {
        obs_data_release(priv);
        return 0;
    }

    int preset = obs_data_get_int(priv, "color-preset");
    obs_data_release(priv);

    return preset;
}


QColor EditorModel::currentItemColor() const
{
    if (!currentItem)
        return QColor();

    obs_data_t *priv = obs_sceneitem_get_private_settings(currentItem);
    if (!priv)
        return QColor();

    const char *colorStr = obs_data_get_string(priv, "color");
    QColor result;

    if (colorStr && *colorStr != 0) {
        result = QColor(QString::fromUtf8(colorStr));
    }

    obs_data_release(priv);
    return result;
}


static bool collect_selected_cb(obs_scene_t *, obs_sceneitem_t *si, void *param)
{
    auto *list = static_cast<QList<obs_sceneitem_t *> *>(param);

    if (obs_sceneitem_selected(si))
        list->append(si);

    obs_source_t *src = obs_sceneitem_get_source(si);
    if (src && obs_source_is_group(src)) {
        obs_scene_t *sub = obs_group_from_source(src);
        if (sub)
            obs_scene_enum_items(sub, collect_selected_cb, param);
    }

    return true;
}

QList<obs_sceneitem_t *> EditorModel::selectedItemsRecursive() const
{
    QList<obs_sceneitem_t *> list;

    if (scene)
        obs_scene_enum_items(scene, collect_selected_cb, &list);

    return list;
}

struct FindByNameCtx {
    const char *name = nullptr;
    obs_sceneitem_t *found = nullptr;
};

static bool find_by_name_cb(obs_scene_t *, obs_sceneitem_t *si, void *param)
{
    auto *ctx = static_cast<FindByNameCtx *>(param);

    obs_source_t *src = obs_sceneitem_get_source(si);
    if (!src)
        return true;

    const char *n = obs_source_get_name(src);
    if (n && strcmp(n, ctx->name) == 0) {
        ctx->found = si;
        return false;
    }

    if (obs_source_is_group(src)) {
        obs_scene_t *sub = obs_group_from_source(src);
        if (sub) {
            FindByNameCtx subCtx;
            subCtx.name = ctx->name;
            obs_scene_enum_items(sub, find_by_name_cb, &subCtx);
            if (subCtx.found) {
                ctx->found = subCtx.found;
                return false;
            }
        }
    }

    return true;
}

static obs_sceneitem_t *find_item_by_name(obs_scene_t *sc, const char *name)
{
    if (!sc || !name || !*name)
        return nullptr;

    FindByNameCtx ctx;
    ctx.name = name;
    obs_scene_enum_items(sc, find_by_name_cb, &ctx);
    return ctx.found;
}

void EditorModel::refreshObsSourcesTree()
{
    auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!mainWindow)
        return;

    QAbstractItemView *tree = mainWindow->findChild<QAbstractItemView *>("sources");
    if (!tree || !tree->model() || !scene)
        return;

    QAbstractItemModel *treeModel = tree->model();

    for (int row = 0; row < treeModel->rowCount(); ++row) {
        const QModelIndex idx = treeModel->index(row, 0);
        QWidget *widget = tree->indexWidget(idx);
        if (!widget)
            continue;

        const QString name = treeModel->data(idx, Qt::AccessibleTextRole).toString();
        obs_sceneitem_t *item = find_item_by_name(scene, name.toUtf8().constData());
        if (!item)
            continue;

        obs_data_t *priv = obs_sceneitem_get_private_settings(item);
        if (!priv)
            continue;

        const int preset = obs_data_get_int(priv, "color-preset");
        const char *color = obs_data_get_string(priv, "color");

        if (preset == 1 && color && *color) {
            widget->setStyleSheet(QStringLiteral("background: ") + color);
        } else if (preset > 1) {
            widget->setStyleSheet(QString());
            widget->setProperty("bgColor", preset - 1);
        } else {
            widget->setStyleSheet("background: none");
            widget->setProperty("bgColor", 0);
        }

        widget->style()->unpolish(widget);
        widget->style()->polish(widget);

        obs_data_release(priv);
    }

    tree->viewport()->update();
}

void EditorModel::applyColorPresetToSelection(int uiPreset)
{
    const QList<obs_sceneitem_t *> items = selectedItemsRecursive();

    for (obs_sceneitem_t *item : items) {
        obs_data_t *priv = obs_sceneitem_get_private_settings(item);
        if (!priv)
            continue;

        if (uiPreset == 0)
            obs_data_set_int(priv, "color-preset", 0);
        else
            obs_data_set_int(priv, "color-preset", uiPreset + 1);

        obs_data_set_string(priv, "color", "");
        obs_data_release(priv);
    }

    refreshObsSourcesTree();
    emit modelUpdated();
}

void EditorModel::applyCustomColorToSelection(const QColor &color)
{
    if (!color.isValid())
        return;

    const QByteArray argb = color.name(QColor::HexArgb).toUtf8();
    const QList<obs_sceneitem_t *> items = selectedItemsRecursive();

    for (obs_sceneitem_t *item : items) {
        obs_data_t *priv = obs_sceneitem_get_private_settings(item);
        if (!priv)
            continue;

        obs_data_set_string(priv, "color", argb.constData());
        obs_data_set_int(priv, "color-preset", 1);
        obs_data_release(priv);
    }

    refreshObsSourcesTree();
    emit modelUpdated();
}

static void e33_apply_bounds_to_item(obs_sceneitem_t *item, obs_bounds_type type)
{
    if (!item)
        return;

    obs_video_info ovi;
    if (!obs_get_video_info(&ovi))
        return;

    obs_transform_info oti;
    obs_sceneitem_get_info2(item, &oti);

    oti.pos.x     = 0.0f;
    oti.pos.y     = 0.0f;
    oti.scale.x   = 1.0f;
    oti.scale.y   = 1.0f;
    oti.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
    oti.rot       = 0.0f;

    oti.bounds_type      = type;
    oti.bounds_alignment = OBS_ALIGN_CENTER;
    oti.bounds.x         = float(ovi.base_width);
    oti.bounds.y         = float(ovi.base_height);
    oti.crop_to_bounds   = obs_sceneitem_get_bounds_crop(item);

    obs_sceneitem_set_info2(item, &oti);
}

obs_data_t *EditorModel::captureTransformState()
{
    obs_data_t *d = obs_data_create();

    if (!currentItem)
        return d;

    obs_scene_t *sc  = obs_sceneitem_get_scene(currentItem);
    obs_source_t *scs = sc ? obs_scene_get_source(sc) : nullptr;

    obs_data_set_string(d, "scene_uuid", scs ? obs_source_get_uuid(scs) : "");
    obs_data_set_int(d, "item_id", obs_sceneitem_get_id(currentItem));

    obs_transform_info oti;
    obs_sceneitem_get_info2(currentItem, &oti);

    obs_data_set_double(d, "pos_x",   oti.pos.x);
    obs_data_set_double(d, "pos_y",   oti.pos.y);
    obs_data_set_double(d, "rot",     oti.rot);
    obs_data_set_double(d, "scale_x", oti.scale.x);
    obs_data_set_double(d, "scale_y", oti.scale.y);
    obs_data_set_int(d, "align", (int64_t)oti.alignment);

    obs_data_set_int(d, "bounds_type", (int64_t)oti.bounds_type);
    obs_data_set_double(d, "bounds_x", oti.bounds.x);
    obs_data_set_double(d, "bounds_y", oti.bounds.y);
    obs_data_set_int(d, "bounds_align", (int64_t)oti.bounds_alignment);
    obs_data_set_bool(d, "crop_to_bounds", oti.crop_to_bounds);

    obs_sceneitem_crop crop;
    obs_sceneitem_get_crop(currentItem, &crop);
    obs_data_set_int(d, "crop_left",   crop.left);
    obs_data_set_int(d, "crop_top",    crop.top);
    obs_data_set_int(d, "crop_right",  crop.right);
    obs_data_set_int(d, "crop_bottom", crop.bottom);

    return d;
}

void EditorModel::applyTransformStateJson(const char *json)
{
    if (!json || !*json)
        return;

    obs_data_t *d = obs_data_create_from_json(json);
    if (!d)
        return;

    const char *uuid = obs_data_get_string(d, "scene_uuid");
    int64_t id = obs_data_get_int(d, "item_id");

    obs_source_t *sceneSrc = (uuid && *uuid) ? obs_get_source_by_uuid(uuid) : nullptr;
    obs_scene_t  *sc       = sceneSrc ? obs_scene_from_source(sceneSrc) : nullptr;
    obs_sceneitem_t *item  = sc ? obs_scene_find_sceneitem_by_id(sc, id) : nullptr;

    bool affectsCurrent = false;

    if (item) {
        obs_transform_info oti;
        obs_sceneitem_get_info2(item, &oti);

        oti.pos.x   = (float)obs_data_get_double(d, "pos_x");
        oti.pos.y   = (float)obs_data_get_double(d, "pos_y");
        oti.rot     = (float)obs_data_get_double(d, "rot");
        oti.scale.x = (float)obs_data_get_double(d, "scale_x");
        oti.scale.y = (float)obs_data_get_double(d, "scale_y");
        oti.alignment = (uint32_t)obs_data_get_int(d, "align");

        oti.bounds_type      = (obs_bounds_type)obs_data_get_int(d, "bounds_type");
        oti.bounds.x         = (float)obs_data_get_double(d, "bounds_x");
        oti.bounds.y         = (float)obs_data_get_double(d, "bounds_y");
        oti.bounds_alignment = (uint32_t)obs_data_get_int(d, "bounds_align");
        oti.crop_to_bounds   = obs_data_get_bool(d, "crop_to_bounds");

        obs_sceneitem_set_info2(item, &oti);

        obs_sceneitem_crop crop;
        crop.left   = (int)obs_data_get_int(d, "crop_left");
        crop.top    = (int)obs_data_get_int(d, "crop_top");
        crop.right  = (int)obs_data_get_int(d, "crop_right");
        crop.bottom = (int)obs_data_get_int(d, "crop_bottom");
        obs_sceneitem_set_crop(item, &crop);

        affectsCurrent = (s_instance && s_instance->currentItem == item);
    }

    if (sceneSrc)
        obs_source_release(sceneSrc);
    obs_data_release(d);

    if (affectsCurrent) {
        s_instance->updateItemData();
        emit s_instance->modelUpdated();
    }
}

void EditorModel::pushTransformUndo(const char *nameKey,
                                    obs_data_t *before, obs_data_t *after,
                                    bool repeatable)
{
    const char *b = obs_data_get_json(before);
    const char *a = obs_data_get_json(after);

    obs_frontend_add_undo_redo_action(obs_module_text(nameKey),
                                      undoStatic, redoStatic,
                                      b ? b : "{}", a ? a : "{}",
                                      repeatable);
}

void EditorModel::undoStatic(const char *data)
{
    if (s_instance)
        s_instance->applyTransformStateJson(data);
}

void EditorModel::redoStatic(const char *data)
{
    if (s_instance)
        s_instance->applyTransformStateJson(data);
}

void EditorModel::runTransformAction(const char *nameKey,
                                     const std::function<void(obs_sceneitem_t *)> &action,
                                     bool repeatable)
{
    if (!currentItem)
        return;

    obs_data_t *before = captureTransformState();
    action(currentItem);
    obs_data_t *after = captureTransformState();

    const char *bj = obs_data_get_json(before);
    const char *aj = obs_data_get_json(after);
    const bool changed = !bj || !aj || strcmp(bj, aj) != 0;

    if (changed) {
        pushTransformUndo(nameKey, before, after, repeatable);

        updateItemData();
        emit modelUpdated();
    }

    obs_data_release(before);
    obs_data_release(after);
}

static void e33_get_item_box(obs_sceneitem_t *item, vec3 &tl, vec3 &br)
{
    matrix4 boxTransform;
    obs_sceneitem_get_box_transform(item, &boxTransform);

    vec3_set(&tl, M_INFINITE, M_INFINITE, 0.0f);
    vec3_set(&br, -M_INFINITE, -M_INFINITE, 0.0f);

    auto getMinPos = [&](float x, float y) {
        vec3 pos;
        vec3_set(&pos, x, y, 0.0f);
        vec3_transform(&pos, &pos, &boxTransform);
        vec3_min(&tl, &tl, &pos);
        vec3_max(&br, &br, &pos);
    };

    getMinPos(0.0f, 0.0f);
    getMinPos(1.0f, 0.0f);
    getMinPos(0.0f, 1.0f);
    getMinPos(1.0f, 1.0f);
}

static vec3 e33_get_item_tl(obs_sceneitem_t *item)
{
    vec3 tl, br;
    e33_get_item_box(item, tl, br);
    return tl;
}

static void e33_rotate_item(obs_sceneitem_t *item, float rot)
{
    const vec3 tl = e33_get_item_tl(item);

    rot += obs_sceneitem_get_rot(item);
    if (rot >= 360.0f)
        rot -= 360.0f;
    else if (rot <= -360.0f)
        rot += 360.0f;
    obs_sceneitem_set_rot(item, rot);
    obs_sceneitem_force_update_transform(item);

    vec3 newTL = e33_get_item_tl(item);
    vec2 pos;
    obs_sceneitem_get_pos(item, &pos);
    pos.x += tl.x - newTL.x;
    pos.y += tl.y - newTL.y;
    obs_sceneitem_set_pos(item, &pos);
}

void EditorModel::rotate90CW()
{
    runTransformAction("UndoRotate90", [](obs_sceneitem_t *item) {
        e33_rotate_item(item, 90.0f);
    });
}

void EditorModel::rotate90CCW()
{
    runTransformAction("UndoRotate90CCW", [](obs_sceneitem_t *item) {
        e33_rotate_item(item, -90.0f);
    });
}

void EditorModel::rotate180()
{
    runTransformAction("UndoRotate180", [](obs_sceneitem_t *item) {
        e33_rotate_item(item, 180.0f);
    });
}

void EditorModel::flipHorizontal()
{
    runTransformAction("UndoFlipH", [](obs_sceneitem_t *item) {
        vec2 scale;
        obs_sceneitem_get_scale(item, &scale);
        scale.x *= -1.0f;
        obs_sceneitem_set_scale(item, &scale);
        obs_sceneitem_force_update_transform(item);
    });
}

void EditorModel::flipVertical()
{
    runTransformAction("UndoFlipV", [](obs_sceneitem_t *item) {
        vec2 scale;
        obs_sceneitem_get_scale(item, &scale);
        scale.y *= -1.0f;
        obs_sceneitem_set_scale(item, &scale);
        obs_sceneitem_force_update_transform(item);
    });
}

void EditorModel::fitToScreen()
{
    runTransformAction("UndoFit", [](obs_sceneitem_t *item) {
        e33_apply_bounds_to_item(item, OBS_BOUNDS_SCALE_INNER);
    });
}

void EditorModel::stretchToScreen()
{
    runTransformAction("UndoStretch", [](obs_sceneitem_t *item) {
        e33_apply_bounds_to_item(item, OBS_BOUNDS_STRETCH);
    });
}

static void e33_center_item_on_screen(obs_sceneitem_t *item, bool centerX, bool centerY)
{
    if (!item)
        return;

    obs_video_info ovi;
    if (!obs_get_video_info(&ovi))
        return;

    obs_transform_info oti;
    obs_sceneitem_get_info2(item, &oti);

    obs_source_t *source = obs_sceneitem_get_source(item);
    if (!source)
        return;

    const float width  = float(obs_source_get_width(source))  * oti.scale.x;
    const float height = float(obs_source_get_height(source)) * oti.scale.y;
    if (width == 0.0f || height == 0.0f)
        return;

    obs_sceneitem_force_update_transform(item);

    vec3 tl, br;
    e33_get_item_box(item, tl, br);

    vec2 pos;
    obs_sceneitem_get_pos(item, &pos);

    if (centerX)
        pos.x += float(ovi.base_width)  * 0.5f - (tl.x + br.x) * 0.5f;
    if (centerY)
        pos.y += float(ovi.base_height) * 0.5f - (tl.y + br.y) * 0.5f;

    obs_sceneitem_set_pos(item, &pos);
}

void EditorModel::centerToScreen()
{
    runTransformAction("UndoCenter", [](obs_sceneitem_t *item) {
        e33_center_item_on_screen(item, true, true);
    });
}

void EditorModel::centerVertical()
{
    runTransformAction("UndoCenter", [](obs_sceneitem_t *item) {
        e33_center_item_on_screen(item, false, true);
    });
}

void EditorModel::centerHorizontal()
{
    runTransformAction("UndoCenter", [](obs_sceneitem_t *item) {
        e33_center_item_on_screen(item, true, false);
    });
}

void EditorModel::copyTransform()
{
    if (editorui::TriggerNativeAction("actionCopyTransform"))
        return;

    if (!currentItem)
        return;

    obs_transform_info oti;
    obs_sceneitem_get_info2(currentItem, &oti);

    if (clipboardTransition)
        obs_data_release(clipboardTransition);

    clipboardTransition = obs_data_create();

    obs_data_set_double(clipboardTransition, "pos_x", oti.pos.x);
    obs_data_set_double(clipboardTransition, "pos_y", oti.pos.y);
    obs_data_set_double(clipboardTransition, "rot",   oti.rot);
    obs_data_set_double(clipboardTransition, "scale_x", oti.scale.x);
    obs_data_set_double(clipboardTransition, "scale_y", oti.scale.y);

    obs_data_set_int(clipboardTransition, "align", oti.alignment);

    obs_sceneitem_crop crop;
    obs_sceneitem_get_crop(currentItem, &crop);

    obs_data_set_int(clipboardTransition, "crop_left",   crop.left);
    obs_data_set_int(clipboardTransition, "crop_top",    crop.top);
    obs_data_set_int(clipboardTransition, "crop_right",  crop.right);
    obs_data_set_int(clipboardTransition, "crop_bottom", crop.bottom);
}

void EditorModel::pasteTransform()
{
    if (editorui::TriggerNativeAction("actionPasteTransform")) {
        emit modelUpdated();
        return;
    }

    if (!clipboardTransition)
        return;

    runTransformAction("UndoPasteTransform", [this](obs_sceneitem_t *item) {
        obs_transform_info oti;
        obs_sceneitem_get_info2(item, &oti);

        oti.pos.x = (float)obs_data_get_double(clipboardTransition, "pos_x");
        oti.pos.y = (float)obs_data_get_double(clipboardTransition, "pos_y");
        oti.rot   = (float)obs_data_get_double(clipboardTransition, "rot");

        oti.scale.x = (float)obs_data_get_double(clipboardTransition, "scale_x");
        oti.scale.y = (float)obs_data_get_double(clipboardTransition, "scale_y");

        oti.alignment = (uint32_t)obs_data_get_int(clipboardTransition, "align");

        obs_sceneitem_set_info2(item, &oti);

        obs_sceneitem_crop crop;
        crop.left   = (int)obs_data_get_int(clipboardTransition, "crop_left");
        crop.top    = (int)obs_data_get_int(clipboardTransition, "crop_top");
        crop.right  = (int)obs_data_get_int(clipboardTransition, "crop_right");
        crop.bottom = (int)obs_data_get_int(clipboardTransition, "crop_bottom");

        obs_sceneitem_set_crop(item, &crop);
    });
}

void EditorModel::resetTransform()
{
    if (editorui::TriggerNativeAction("actionResetTransform")) {
        emit modelUpdated();
        return;
    }

    runTransformAction("UndoResetTransform", [](obs_sceneitem_t *item) {
        obs_transform_info oti;
        obs_sceneitem_get_info2(item, &oti);

        oti.pos.x = 0.0f;
        oti.pos.y = 0.0f;
        oti.rot   = 0.0f;

        oti.scale.x = 1.0f;
        oti.scale.y = 1.0f;

        oti.bounds_type = OBS_BOUNDS_NONE;
        oti.bounds.x = 0.0f;
        oti.bounds.y = 0.0f;
        oti.bounds_alignment = 0;
        oti.crop_to_bounds = false;

        obs_sceneitem_set_info2(item, &oti);

        obs_sceneitem_crop crop = {};
        crop.left = crop.top = crop.right = crop.bottom = 0;
        obs_sceneitem_set_crop(item, &crop);
    });
}

void EditorModel::editTransform(const char *nameKey,
                                const std::function<void(obs_sceneitem_t *)> &action)
{
    runTransformAction(nameKey, action, true);
}


static void item_prop_cb(const char *data)
{
    obs_data_t *d = obs_data_create_from_json(data);
    if (!d)
        return;

    const char *sceneUuid = obs_data_get_string(d, "scene_uuid");
    int64_t itemId = obs_data_get_int(d, "item_id");
    const char *prop = obs_data_get_string(d, "prop");
    int value = (int)obs_data_get_int(d, "value");

    obs_source_t *sceneSrc =
        (sceneUuid && *sceneUuid) ? obs_get_source_by_uuid(sceneUuid) : nullptr;
    obs_scene_t *sc = sceneSrc ? obs_scene_from_source(sceneSrc) : nullptr;
    obs_sceneitem_t *item = sc ? obs_scene_find_sceneitem_by_id(sc, itemId) : nullptr;

    if (item && prop && *prop) {
        if (!strcmp(prop, "scale_filter"))
            obs_sceneitem_set_scale_filter(item, (obs_scale_type)value);
        else if (!strcmp(prop, "blend_mode"))
            obs_sceneitem_set_blending_mode(item, (obs_blending_type)value);
        else if (!strcmp(prop, "blend_method"))
            obs_sceneitem_set_blending_method(item, (obs_blending_method)value);
        else if (!strcmp(prop, "transition_duration_show"))
            obs_sceneitem_set_transition_duration(item, true, (uint32_t)value);
        else if (!strcmp(prop, "transition_duration_hide"))
            obs_sceneitem_set_transition_duration(item, false, (uint32_t)value);
    }

    if (sceneSrc)
        obs_source_release(sceneSrc);
    obs_data_release(d);

    if (EditorModel *m = EditorModel::instanceForUndo()) {
        m->refreshItemCaches();
        m->modelUpdated();
    }
}

static void source_prop_cb(const char *data)
{
    obs_data_t *d = obs_data_create_from_json(data);
    if (!d)
        return;

    const char *uuid = obs_data_get_string(d, "uuid");
    const char *prop = obs_data_get_string(d, "prop");
    double value = obs_data_get_double(d, "value");

    obs_source_t *src = (uuid && *uuid) ? obs_get_source_by_uuid(uuid) : nullptr;

    if (src && prop && *prop) {
        if (!strcmp(prop, "deinterlace_mode"))
            obs_source_set_deinterlace_mode(src, (obs_deinterlace_mode)(int)value);
        else if (!strcmp(prop, "deinterlace_order"))
            obs_source_set_deinterlace_field_order(
                src, (obs_deinterlace_field_order)(int)value);
    }

    if (src)
        obs_source_release(src);
    obs_data_release(d);

    if (EditorModel *m = EditorModel::instanceForUndo())
        m->modelUpdated();
}

static void audio_state_cb(const char *data)
{
    obs_data_t *d = obs_data_create_from_json(data);
    if (!d)
        return;

    const char *uuid = obs_data_get_string(d, "uuid");
    obs_source_t *src = (uuid && *uuid) ? obs_get_source_by_uuid(uuid) : nullptr;

    if (src) {
        obs_data_t *priv = obs_source_get_private_settings(src);
        obs_data_set_bool(priv, "mixer_hidden", obs_data_get_bool(d, "mixer_hidden"));
        obs_data_release(priv);

        obs_source_set_sync_offset(src, obs_data_get_int(d, "sync_ns"));

        uint32_t flags = obs_source_get_flags(src);
        if (obs_data_get_bool(d, "mono"))
            flags |= OBS_SOURCE_FLAG_FORCE_MONO;
        else
            flags &= ~OBS_SOURCE_FLAG_FORCE_MONO;
        obs_source_set_flags(src, flags);

        obs_source_set_balance_value(src, (float)obs_data_get_double(d, "balance"));
        obs_source_set_monitoring_enabled(
            src, obs_data_get_int(d, "monitoring") != 0);
        obs_source_set_audio_mixers(src, (uint32_t)obs_data_get_int(d, "mixers"));

        obs_source_release(src);
    }

    obs_data_release(d);

    if (EditorModel *m = EditorModel::instanceForUndo())
        m->modelUpdated();
}

obs_data_t *EditorModel::captureAudioState(obs_source_t *src)
{
    obs_data_t *d = obs_data_create();
    if (!src)
        return d;

    obs_data_set_string(d, "uuid", obs_source_get_uuid(src));

    obs_data_t *priv = obs_source_get_private_settings(src);
    obs_data_set_bool(d, "mixer_hidden", obs_data_get_bool(priv, "mixer_hidden"));
    obs_data_release(priv);

    obs_data_set_int(d, "sync_ns", obs_source_get_sync_offset(src));
    obs_data_set_bool(d, "mono",
                      (obs_source_get_flags(src) & OBS_SOURCE_FLAG_FORCE_MONO) != 0);
    obs_data_set_double(d, "balance", obs_source_get_balance_value(src));
    obs_data_set_int(d, "monitoring",
                     obs_source_get_monitoring_enabled(src) ? 1 : 0);
    obs_data_set_int(d, "mixers", obs_source_get_audio_mixers(src));

    return d;
}

void EditorModel::pushAudioUndo(const char *nameKey, obs_data_t *before,
                                obs_data_t *after, bool repeatable)
{
    const char *b = before ? obs_data_get_json(before) : nullptr;
    const char *a = after ? obs_data_get_json(after) : nullptr;

    if (!b || !a || strcmp(b, a) == 0)
        return;

    obs_frontend_add_undo_redo_action(obs_module_text(nameKey),
                                      audio_state_cb, audio_state_cb,
                                      b, a, repeatable);
}

void EditorModel::pushItemPropUndo(const char *nameKey, obs_sceneitem_t *item,
                                   const char *prop, int before, int after,
                                   bool repeatable)
{
    if (!item || before == after)
        return;

    obs_scene_t *sc = obs_sceneitem_get_scene(item);
    obs_source_t *scs = sc ? obs_scene_get_source(sc) : nullptr;

    obs_data_t *b = obs_data_create();
    obs_data_set_string(b, "scene_uuid", scs ? obs_source_get_uuid(scs) : "");
    obs_data_set_int(b, "item_id", obs_sceneitem_get_id(item));
    obs_data_set_string(b, "prop", prop);
    obs_data_set_int(b, "value", before);

    obs_data_t *a = obs_data_create();
    obs_data_set_string(a, "scene_uuid", scs ? obs_source_get_uuid(scs) : "");
    obs_data_set_int(a, "item_id", obs_sceneitem_get_id(item));
    obs_data_set_string(a, "prop", prop);
    obs_data_set_int(a, "value", after);

    const char *bj = obs_data_get_json(b);
    const char *aj = obs_data_get_json(a);

    obs_frontend_add_undo_redo_action(obs_module_text(nameKey),
                                      item_prop_cb, item_prop_cb,
                                      bj ? bj : "{}", aj ? aj : "{}",
                                      repeatable);

    obs_data_release(b);
    obs_data_release(a);
}

void EditorModel::pushSourcePropUndo(const char *nameKey, obs_source_t *src,
                                     const char *prop, double before, double after,
                                     bool repeatable)
{
    if (!src || before == after)
        return;

    obs_data_t *b = obs_data_create();
    obs_data_set_string(b, "uuid", obs_source_get_uuid(src));
    obs_data_set_string(b, "prop", prop);
    obs_data_set_double(b, "value", before);

    obs_data_t *a = obs_data_create();
    obs_data_set_string(a, "uuid", obs_source_get_uuid(src));
    obs_data_set_string(a, "prop", prop);
    obs_data_set_double(a, "value", after);

    const char *bj = obs_data_get_json(b);
    const char *aj = obs_data_get_json(a);

    obs_frontend_add_undo_redo_action(obs_module_text(nameKey),
                                      source_prop_cb, source_prop_cb,
                                      bj ? bj : "{}", aj ? aj : "{}",
                                      repeatable);

    obs_data_release(b);
    obs_data_release(a);
}
void EditorModel::openNativeProperties()
{
    if (currentSource)
        obs_frontend_open_source_properties(currentSource);
}

void EditorModel::openNativeSourceFilters()
{
    if (currentSource)
        obs_frontend_open_source_filters(currentSource);
}

void EditorModel::openNativeTransform()
{
    if (currentItem)
        obs_frontend_open_sceneitem_edit_transform(currentItem);
}
