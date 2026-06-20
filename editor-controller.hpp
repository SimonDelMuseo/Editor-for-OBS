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
    void handleSelectionPoll();

private:
    EditorDock  *dock      = nullptr;
    EditorModel *model     = nullptr;
    QTimer      *pollTimer = nullptr;

    void handleSceneChanged();

    static void frontend_event_callback(enum obs_frontend_event event,
                                        void *userdata);
};
