#ifndef E33_EDITOR_MODEL_HPP
#define E33_EDITOR_MODEL_HPP

#include <QObject>
#include <QString>
#include <QColor>
#include <QList>
#include <obs.h>
#include <functional>

class EditorModel : public QObject {
    Q_OBJECT

public:
    explicit EditorModel(QObject *parent = nullptr);
    ~EditorModel();

    void setSelectedItem(obs_sceneitem_t *item);
    void setCurrentScene(obs_scene_t *scene);

    void syncSelection(obs_sceneitem_t *item);

    void refreshItemCaches();
    void refreshSceneCaches();
    void notifySceneListChanged();
    void notifySceneItemsChanged();
    void notifyTransformChanged();
    void notifyFiltersChanged();
    void invalidateUsedInCache();

    static void sourceStateCb(void *param, calldata_t *data);
    static void sourceDestroyCb(void *param, calldata_t *data);
    void connectSourceSignals();
    void disconnectSourceSignals();
    obs_source_t *connectedSource = nullptr;

    void invalidate();

    static EditorModel *instanceForUndo() { return s_instance; }
    void applyTransformStateJson(const char *json);

    void openNativeProperties();
    void openNativeSourceFilters();
    void openNativeTransform();

    obs_sceneitem_t *currentSceneItem() const { return currentItem; }

    obs_sceneitem_t *selectedItem() const { return currentItem; }
    obs_source_t    *selectedSource() const { return currentSource; }

    QString sourceName() const { return sourceNameCache; }
    QString sourceType() const { return sourceTypeCache; }
    bool isVisible() const { return visibleCache; }
    bool isLocked() const { return lockedCache; }

    QColor currentItemColor() const;
    int currentItemPreset() const;

    void applyColorPresetToSelection(int uiPreset);
    void applyCustomColorToSelection(const QColor &color);

    QString sourceSize() const;
    QString scaledSize() const;
    QString usedInScenes() const;

    obs_scene_t  *currentScene() const { return scene; }
    obs_source_t *currentSceneSource() const { return sceneSource; }
    QString       currentSceneName() const { return sceneNameCache; }

    QString sceneName() const { return sceneNameCache; }
    QString sourceNameCacheValue() const { return sourceNameCache; }
    obs_source_t *sceneSourcePtr() const { return sceneSource; }
    obs_source_t *currentSourcePtr() const { return currentSource; }

    obs_data_t *clipboardTransition = nullptr;

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

    void copyTransform();
    void pasteTransform();
    void resetTransform();

    void editTransform(const char *nameKey,
                       const std::function<void(obs_sceneitem_t *)> &action);

    obs_data_t *captureAudioState(obs_source_t *src);
    void pushAudioUndo(const char *nameKey, obs_data_t *before,
                       obs_data_t *after, bool repeatable = true);
    void pushItemPropUndo(const char *nameKey, obs_sceneitem_t *item,
                          const char *prop, int before, int after,
                          bool repeatable = false);
    void pushSourcePropUndo(const char *nameKey, obs_source_t *src,
                            const char *prop, double before, double after,
                            bool repeatable = false);

signals:
    void modelUpdated();
    void sceneChanged();
    void sceneListChanged();
    void sceneItemsChanged();
    void transformChanged();
    void filtersChangedExternally();

private:
    obs_sceneitem_t *currentItem  = nullptr;
    obs_source_t    *currentSource = nullptr;

    QString sourceNameCache;
    QString sourceTypeCache;
    bool visibleCache = false;
    bool lockedCache  = false;

    obs_scene_t  *scene       = nullptr;
    obs_source_t *sceneSource = nullptr;
    QString       sceneNameCache;

    mutable bool          usedInValid = false;
    mutable obs_source_t *usedInSource = nullptr;
    mutable int           usedInCount  = 0;

    static EditorModel *s_instance;

    void updateItemData();
    void updateSceneData();

    QList<obs_sceneitem_t *> selectedItemsRecursive() const;
    void refreshObsSourcesTree();

    void releaseItemRefs();
    void releaseSceneRefs();

    obs_data_t *captureTransformState();
    void pushTransformUndo(const char *nameKey, obs_data_t *before, obs_data_t *after,
                           bool repeatable = false);
    void runTransformAction(const char *nameKey,
                            const std::function<void(obs_sceneitem_t *)> &action,
                            bool repeatable = false);
    static void undoStatic(const char *data);
    static void redoStatic(const char *data);
};

#endif
