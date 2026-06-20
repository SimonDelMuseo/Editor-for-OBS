/* ────────────────────────────────────────────────
   INCLUDES
   ──────────────────────────────────────────────── */
#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QApplication>
#include <QMainWindow>

#include "plugin.hpp"
#include "editor-dock.hpp"
#include "editor-controller.hpp"
#include "editor-model.hpp"

/* ────────────────────────────────────────────────
   VARIABLES GLOBALES
   ──────────────────────────────────────────────── */
static EditorController *controller = nullptr;
static EditorDock       *editorDock = nullptr;
static EditorModel      *model      = nullptr;

/* ────────────────────────────────────────────────
   DECLARACIÓN DEL MÓDULO
   ──────────────────────────────────────────────── */
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("E33-Editor", "en-US")

/* ────────────────────────────────────────────────
   LOAD — REGISTRO EXACTO AL MODELO EXELDRO
   ──────────────────────────────────────────────── */
bool obs_module_load(void)
{
    blog(LOG_INFO, "[E33 Editor] Loaded");

    if (!QApplication::instance()) {
        int argc = 0;
        char **argv = nullptr;
        new QApplication(argc, argv);
    }

    model = new EditorModel();

    // Igual que Exeldro: parent = QMainWindow de OBS
    auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    editorDock = new EditorDock(model, mainWindow);

    controller = new EditorController(editorDock, model);

    obs_frontend_add_dock_by_id(
        "E33-Editor",
        obs_module_text("E33PanelName"),
        editorDock
    );

    return true;
}

/* ────────────────────────────────────────────────
   UNLOAD
   ──────────────────────────────────────────────── */
void obs_module_unload(void)
{
    blog(LOG_INFO, "[E33 Editor] Unloaded");

    delete controller;
    controller = nullptr;

    delete model;
    model = nullptr;

    delete editorDock;
    editorDock = nullptr;
}
