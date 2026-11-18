#pragma once

#include <QObject>
#include <QTimer>

extern "C" {
#include <obs-frontend-api.h>
#include <obs.h>
}

class EditorDock;
class EditorModel;

class EditorController : public QObject
{
    Q_OBJECT

public:
    explicit EditorController(EditorDock *dock,
                              EditorModel *model,
                              QObject *parent = nullptr);

    ~EditorController();

private slots:
    void handleSafetyPoll();

private:
    EditorDock  *dock      = nullptr;
    EditorModel *model     = nullptr;

    QTimer      *pollTimer = nullptr;

    obs_source_t *boundSceneSource = nullptr;

    void handleSceneChanged();

    void connectSceneSignals();
    void disconnectSceneSignals();
    void connectGlobalSignals();
    void disconnectGlobalSignals();

    obs_source_t *acquireActiveScene() const;

    void rescanSelection();

    static void frontend_event_callback(enum obs_frontend_event event,
                                        void *userdata);

    static void scene_item_select_cb(void *data, calldata_t *cd);
    static void scene_item_deselect_cb(void *data, calldata_t *cd);
    static void scene_item_remove_cb(void *data, calldata_t *cd);
    static void scene_item_add_cb(void *data, calldata_t *cd);
    static void scene_item_transform_cb(void *data, calldata_t *cd);
    static void scene_item_state_cb(void *data, calldata_t *cd);

    static void global_filter_changed_cb(void *data, calldata_t *cd);
};
