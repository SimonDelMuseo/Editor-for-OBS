#ifndef E33_EDITOR_MODEL_HPP
#define E33_EDITOR_MODEL_HPP

#include <QObject>
#include <QString>
#include <QColor> 
#include <obs.h>

class EditorModel : public QObject {
    Q_OBJECT

public:
    explicit EditorModel(QObject *parent = nullptr);
    ~EditorModel();

    /* ────────────────────────────────────────────────
       ACTUALIZACIÓN DEL MODELO
       ──────────────────────────────────────────────── */
    void setSelectedItem(obs_sceneitem_t *item);
    void setCurrentScene(obs_scene_t *scene);

    /* ────────────────────────────────────────────────
       GETTERS PÚBLICOS — ESCENA ACTUAL Y FUENTE ACTUAL
       ──────────────────────────────────────────────── */
    obs_sceneitem_t *currentSceneItem() const { return currentItem; }

    /* ────────────────────────────────────────────────
       GETTERS PÚBLICOS — ITEM SELECCIONADO
       ──────────────────────────────────────────────── */
    obs_sceneitem_t *selectedItem() const { return currentItem; }
    obs_source_t    *selectedSource() const { return currentSource; }

    QString sourceName() const { return sourceNameCache; }
    QString sourceType() const { return sourceTypeCache; }
    bool isVisible() const { return visibleCache; }
    bool isLocked() const { return lockedCache; }

    /* ────────────────────────────────────────────────
      GETTERS PÚBLICOS — COLOR DEL ITEM 
      ──────────────────────────────────────────────── */
    QColor currentItemColor() const;
    int currentItemPreset() const;

    /* ────────────────────────────────────────────────
       NUEVOS GETTERS — BASIC INFORMATION
       ──────────────────────────────────────────────── */
    QString sourceSize() const;
    QString scaledSize() const;
    QString usedInScenes() const;

    /* ────────────────────────────────────────────────
       GETTERS PÚBLICOS — ESCENA ACTIVA
       ──────────────────────────────────────────────── */
    obs_scene_t  *currentScene() const { return scene; }
    obs_source_t *currentSceneSource() const { return sceneSource; }
    QString       currentSceneName() const { return sceneNameCache; }

    /* ────────────────────────────────────────────────
       GETTERS AÑADIDOS — PARA HEADER (NOMBRES Y PUNTEROS)
       ──────────────────────────────────────────────── */
    QString sceneName() const { return sceneNameCache; }
    QString sourceNameCacheValue() const { return sourceNameCache; }
    obs_source_t *sceneSourcePtr() const { return sceneSource; }
    obs_source_t *currentSourcePtr() const { return currentSource; }

    /* ────────────────────────────────────────────────
       CLIPBOARD — TRANSICIONES (COPIAR/PEGAR)
       ──────────────────────────────────────────────── */
    obs_data_t *clipboardTransition = nullptr;

    /* ────────────────────────────────────────────────
       ACCIONES DE TRANSFORMACIÓN (NUEVO)
       ──────────────────────────────────────────────── */
    void rotate90CW();
    void rotate90CCW();
    void rotate180();

    void flipHorizontal();
    void flipVertical();

    void fitToScreen();
    void stretchToScreen();

    void centerToScreen();
    void centerVertical();
    void centerHorizontal();

    /* ────────────────────────────────────────────────
       COPY / PASTE / RESET (NUEVO)
       ──────────────────────────────────────────────── */
    void copyTransform();
    void pasteTransform();
    void resetTransform();

signals:
    void modelUpdated();
    void sceneChanged();

private:
    /* ────────────────────────────────────────────────
       Datos internos — ITEM SELECCIONADO
       ──────────────────────────────────────────────── */
    obs_sceneitem_t *currentItem  = nullptr;
    obs_source_t    *currentSource = nullptr;

    QString sourceNameCache;
    QString sourceTypeCache;
    bool visibleCache = false;
    bool lockedCache  = false;

    /* ────────────────────────────────────────────────
       Datos internos — ESCENA ACTIVA
       ──────────────────────────────────────────────── */
    obs_scene_t  *scene       = nullptr;
    obs_source_t *sceneSource = nullptr;
    QString       sceneNameCache;

    /* ────────────────────────────────────────────────
       Funciones internas para refrescar datos
       ──────────────────────────────────────────────── */
    void updateItemData();
    void updateSceneData();
};

#endif // E33_EDITOR_MODEL_HPP
