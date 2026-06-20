#include "editor-model.hpp"
#include "obs-module.h"
#include <obs.h>
#include <obs-data.h>
#include <QColor>

/* ────────────────────────────────────────────────
   ESTRUCTURAS AUXILIARES PARA "USED IN"
   ──────────────────────────────────────────────── */
struct UsedInData {
    obs_source_t *target = nullptr;
    int count = 0;
};

struct ItemSearchData {
    obs_source_t *target = nullptr;
    bool found = false;
};

/* ────────────────────────────────────────────────
   BÚSQUEDA RECURSIVA EN UN ITEM (SOPORTA GRUPOS)
   ──────────────────────────────────────────────── */
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
        return false; // parar enumeración
    }

    return true; // seguir
}

static bool search_in_item(obs_sceneitem_t *item, obs_source_t *target)
{
    if (!item || !target)
        return false;

    obs_source_t *itemSource = obs_sceneitem_get_source(item);
    if (!itemSource)
        return false;

    /* ✔️ Coincide directamente con la fuente objetivo */
    if (itemSource == target)
        return true;

    /* ✔️ Si es un grupo, recorrer sus hijos recursivamente */
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

/* ────────────────────────────────────────────────
   CALLBACK: ENUMERAR ITEMS DE UNA ESCENA
   ──────────────────────────────────────────────── */
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
        return false; // parar enumeración
    }

    return true; // seguir
}

/* ────────────────────────────────────────────────
   CALLBACK: ENUMERAR ESCENAS
   ──────────────────────────────────────────────── */
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

    return true; // seguir enumerando escenas
}

/* ────────────────────────────────────────────────
   CONSTRUCTOR / DESTRUCTOR
   ──────────────────────────────────────────────── */
EditorModel::EditorModel(QObject *parent)
    : QObject(parent)
{
}

EditorModel::~EditorModel()
{
}

/* ────────────────────────────────────────────────
   ACTUALIZAR ITEM SELECCIONADO
   ──────────────────────────────────────────────── */
void EditorModel::setSelectedItem(obs_sceneitem_t *item)
{
    currentItem = item;

    if (currentItem)
        currentSource = obs_sceneitem_get_source(currentItem);
    else
        currentSource = nullptr;

    /* 🔥 NUEVO: refrescar datos antes de emitir la señal */
    updateItemData();
    updateSceneData();

    emit modelUpdated();   // ← CLAVE: actualiza la UI
}

/* ────────────────────────────────────────────────
   ACTUALIZAR ESCENA ACTIVA
   ──────────────────────────────────────────────── */
void EditorModel::setCurrentScene(obs_scene_t *newScene)
{
    scene = newScene;

    if (scene)
        sceneSource = obs_scene_get_source(scene);
    else
        sceneSource = nullptr;

    updateSceneData();
    emit sceneChanged();
}

/* ────────────────────────────────────────────────
   REFRESCAR DATOS DEL ITEM SELECCIONADO
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   REFRESCAR DATOS DE LA ESCENA ACTIVA
   ──────────────────────────────────────────────── */
void EditorModel::updateSceneData()
{
    if (!sceneSource) {
        sceneNameCache.clear();
        return;
    }

    const char *name = obs_source_get_name(sceneSource);
    sceneNameCache = name ? QString::fromUtf8(name) : QString();
}

/* ────────────────────────────────────────────────
   SOURCE ORIGINAL SIZE (WIDTH x HEIGHT)
   ──────────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────
   SOURCE SCALED SIZE (WIDTH x HEIGHT)
   ──────────────────────────────────────────────── */
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

    // 🔥 1) Si hay bounds activos (fit, stretch, ctrl+F, ctrl+S)
    if (oti.bounds_type != OBS_BOUNDS_NONE) {

        // Si crop_to_bounds está activo, OBS recorta al tamaño del lienzo
        if (oti.crop_to_bounds) {
            return QString("%1 x %2")
                .arg(int(oti.bounds.x))
                .arg(int(oti.bounds.y));
        }

        // Si no hay crop, el tamaño final es el área de bounds
        return QString("%1 x %2")
            .arg(int(oti.bounds.x))
            .arg(int(oti.bounds.y));
    }

    // 🔥 2) Si NO hay bounds → usar escala normal
    vec2 scale;
    obs_sceneitem_get_scale(currentItem, &scale);

    int scaledW = int(w * scale.x);
    int scaledH = int(h * scale.y);

    return QString("%1 x %2").arg(scaledW).arg(scaledH);
}

/* ────────────────────────────────────────────────
   USED IN — CUÁNTAS ESCENAS CONTIENEN ESTA FUENTE
   (INCLUYENDO GRUPOS Y SUBGRUPOS)
   ──────────────────────────────────────────────── */
QString EditorModel::usedInScenes() const
{
    if (!currentSource)
        return "-";

    UsedInData data;
    data.target = currentSource;
    data.count  = 0;

    obs_enum_scenes(enum_scenes_callback, &data);

    QString word = (data.count == 1)
    ? QString(obs_module_text("SceneSingular"))
    : QString(obs_module_text("ScenePlural"));

    return QString("%1 %2").arg(data.count).arg(word);
}

/* ────────────────────────────────────────────────
   COLOR REAL QUE USA OBS PARA MARCAR LA FUENTE
   ──────────────────────────────────────────────── */
int EditorModel::currentItemPreset() const
{
    if (!currentItem)
        return 0;

    obs_data_t *priv = obs_sceneitem_get_private_settings(currentItem);
    if (!priv)
        return 0;

    // 1) Si hay color real (ARGB), consideramos "custom" → NO marcar ningún preset
    const char *colorStr = obs_data_get_string(priv, "color");
    if (colorStr && *colorStr != 0) {
        obs_data_release(priv);
        return 0;  // custom color → ningún botón de preset activo
    }

    // 2) Si no hay color real → usar el preset normal
    int preset = obs_data_get_int(priv, "color-preset");
    obs_data_release(priv);

    return preset;  // 0 = sin color, 1..8 = presets OBS
}


/* ────────────────────────────────────────────────
   COLOR REAL ARGB DEL ITEM (igual que OBS)
   ──────────────────────────────────────────────── */
QColor EditorModel::currentItemColor() const
{
    if (!currentItem)
        return QColor();  // sin color

    obs_data_t *priv = obs_sceneitem_get_private_settings(currentItem);
    if (!priv)
        return QColor();

    const char *colorStr = obs_data_get_string(priv, "color");
    QColor result;

    if (colorStr && *colorStr != 0) {
        // Color ARGB real
        result = QColor(QString::fromUtf8(colorStr));
    }

    obs_data_release(priv);
    return result;
}

/* ────────────────────────────────────────────────
   HELPERS INTERNOS PARA TRANSFORMACIONES
   (NO TOCAN NADA EXTERNO)
   ──────────────────────────────────────────────── */
static void e33_center_item_on_screen(obs_sceneitem_t *item, bool centerX, bool centerY)
{
    if (!item)
        return;

    obs_video_info ovi;
    if (!obs_get_video_info(&ovi))
        return;

    obs_transform_info oti;
    obs_sceneitem_get_info2(item, &oti);

    float cx = float(ovi.base_width)  * 0.5f;
    float cy = float(ovi.base_height) * 0.5f;

    if (centerX)
        oti.pos.x = cx;
    if (centerY)
        oti.pos.y = cy;

    oti.alignment = OBS_ALIGN_CENTER;

    obs_sceneitem_set_info2(item, &oti);
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

    oti.bounds_type      = type;
    oti.bounds_alignment = OBS_ALIGN_CENTER;
    oti.bounds.x         = float(ovi.base_width);
    oti.bounds.y         = float(ovi.base_height);
    oti.crop_to_bounds   = obs_sceneitem_get_bounds_crop(item);

    oti.alignment = OBS_ALIGN_CENTER;
    oti.pos.x     = float(ovi.base_width)  * 0.5f;
    oti.pos.y     = float(ovi.base_height) * 0.5f;

    obs_sceneitem_set_info2(item, &oti);
}

/* ────────────────────────────────────────────────
   ACCIONES DE TRANSFORMACIÓN — ROTATE
   ──────────────────────────────────────────────── */
void EditorModel::rotate90CW()
{
    if (!currentItem)
        return;

    obs_transform_info oti;
    obs_sceneitem_get_info2(currentItem, &oti);
    oti.rot += 90.0f;
    obs_sceneitem_set_info2(currentItem, &oti);

    updateItemData();
    emit modelUpdated();
}

void EditorModel::rotate90CCW()
{
    if (!currentItem)
        return;

    obs_transform_info oti;
    obs_sceneitem_get_info2(currentItem, &oti);
    oti.rot -= 90.0f;
    obs_sceneitem_set_info2(currentItem, &oti);

    updateItemData();
    emit modelUpdated();
}

void EditorModel::rotate180()
{
    if (!currentItem)
        return;

    obs_transform_info oti;
    obs_sceneitem_get_info2(currentItem, &oti);
    oti.rot += 180.0f;
    obs_sceneitem_set_info2(currentItem, &oti);

    updateItemData();
    emit modelUpdated();
}

/* ────────────────────────────────────────────────
   ACCIONES DE TRANSFORMACIÓN — FLIP
   ──────────────────────────────────────────────── */
void EditorModel::flipHorizontal()
{
    if (!currentItem)
        return;

    vec2 scale;
    obs_sceneitem_get_scale(currentItem, &scale);
    scale.x *= -1.0f;
    obs_sceneitem_set_scale(currentItem, &scale);
    obs_sceneitem_force_update_transform(currentItem);

    updateItemData();
    emit modelUpdated();
}

void EditorModel::flipVertical()
{
    if (!currentItem)
        return;

    vec2 scale;
    obs_sceneitem_get_scale(currentItem, &scale);
    scale.y *= -1.0f;
    obs_sceneitem_set_scale(currentItem, &scale);
    obs_sceneitem_force_update_transform(currentItem);

    updateItemData();
    emit modelUpdated();
}

/* ────────────────────────────────────────────────
   ACCIONES DE TRANSFORMACIÓN — FIT / STRETCH
   ──────────────────────────────────────────────── */
void EditorModel::fitToScreen()
{
    if (!currentItem)
        return;

    e33_apply_bounds_to_item(currentItem, OBS_BOUNDS_SCALE_INNER);

    updateItemData();
    emit modelUpdated();
}

void EditorModel::stretchToScreen()
{
    if (!currentItem)
        return;

    e33_apply_bounds_to_item(currentItem, OBS_BOUNDS_STRETCH);

    updateItemData();
    emit modelUpdated();
}

/* ────────────────────────────────────────────────
   ACCIONES DE TRANSFORMACIÓN — CENTER
   ──────────────────────────────────────────────── */
void EditorModel::centerToScreen()
{
    if (!currentItem)
        return;

    e33_center_item_on_screen(currentItem, true, true);

    updateItemData();
    emit modelUpdated();
}

void EditorModel::centerVertical()
{
    if (!currentItem)
        return;

    e33_center_item_on_screen(currentItem, false, true);

    updateItemData();
    emit modelUpdated();
}

void EditorModel::centerHorizontal()
{
    if (!currentItem)
        return;

    e33_center_item_on_screen(currentItem, true, false);

    updateItemData();
    emit modelUpdated();
}

/* ────────────────────────────────────────────────
   COPY TRANSFORM
   ──────────────────────────────────────────────── */
void EditorModel::copyTransform()
{
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

/* ────────────────────────────────────────────────
   PASTE TRANSFORM
   ──────────────────────────────────────────────── */
void EditorModel::pasteTransform()
{
    if (!currentItem || !clipboardTransition)
        return;

    obs_transform_info oti;
    obs_sceneitem_get_info2(currentItem, &oti);

    oti.pos.x = obs_data_get_double(clipboardTransition, "pos_x");
    oti.pos.y = obs_data_get_double(clipboardTransition, "pos_y");
    oti.rot   = obs_data_get_double(clipboardTransition, "rot");

    oti.scale.x = obs_data_get_double(clipboardTransition, "scale_x");
    oti.scale.y = obs_data_get_double(clipboardTransition, "scale_y");

    oti.alignment = (uint32_t)obs_data_get_int(clipboardTransition, "align");

    obs_sceneitem_set_info2(currentItem, &oti);

    obs_sceneitem_crop crop;
    crop.left   = obs_data_get_int(clipboardTransition, "crop_left");
    crop.top    = obs_data_get_int(clipboardTransition, "crop_top");
    crop.right  = obs_data_get_int(clipboardTransition, "crop_right");
    crop.bottom = obs_data_get_int(clipboardTransition, "crop_bottom");

    obs_sceneitem_set_crop(currentItem, &crop);

    setSelectedItem(currentItem);
}

/* ────────────────────────────────────────────────
   RESET TRANSFORM (compatible OBS 32.1)
   ──────────────────────────────────────────────── */
void EditorModel::resetTransform()
{
    if (!currentItem)
        return;

    /* Reset transform info */
    obs_transform_info oti;
    obs_sceneitem_get_info2(currentItem, &oti);

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

    obs_sceneitem_set_info2(currentItem, &oti);

    /* Reset crop */
    obs_sceneitem_crop crop = {};
    crop.left = crop.top = crop.right = crop.bottom = 0;
    obs_sceneitem_set_crop(currentItem, &crop);

    /* Notificar */
    setSelectedItem(currentItem);
}
