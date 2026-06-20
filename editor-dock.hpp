/* ────────────────────────────────────────────────
   EDITOR-DOCK.HPP — DOCK PADRE TIPO EXELDRO (QFRAME)
   ──────────────────────────────────────────────── */
#pragma once

#include <QFrame>
#include <QList>
#include <QPointer>

class EditorWidget;
class EditorModel;
class Information;
class FiltersHeader;

class EditorDock : public QFrame
{
    Q_OBJECT

public:
    explicit EditorDock(EditorModel *model, QWidget *parent = nullptr);
    ~EditorDock();   // ← NUEVO

    // ← NUEVO: registrar popouts creados
    void registerPopout(QWidget *w);

private:
    EditorWidget *editorWidget = nullptr;
    EditorModel  *model        = nullptr;
    Information *information   = nullptr;

    QWidget *topBar     = nullptr;
    QWidget *bottomBar  = nullptr;

    FiltersHeader *filtersHeader = nullptr;

    // ← NUEVO: lista de popouts abiertos
    QList<QPointer<QWidget>> openPopouts;

    void refreshUI();

private slots:
    void onTabChanged(int index);
};
