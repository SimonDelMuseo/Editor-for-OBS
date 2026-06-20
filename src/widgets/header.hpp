#pragma once
#include <QFrame>

class QComboBox;
class QPushButton;
class EditorModel;

class Header : public QFrame
{
    Q_OBJECT

public:
    explicit Header(EditorModel *model, QWidget *parent = nullptr);
    
    //BOTONES CAMBIANDO DE PESTAÑAS EN EL HEADER
    QPushButton *getSourcePanelButton() const { return sourceIconBtn; }
    QPushButton *getPropertiesPanelButton() const { return sourcePropertiesPanelBtn; }
    QPushButton *getFilterPanelButton() const { return sourceFiltersPanelBtn; }
    QPushButton *getSceneFiltersPanelButton() const { return sceneFilterPanelBtn; }
    QPushButton *getTransformPanelButton() const { return sourceTransformPanelBtn; }
    
    void updatePanelButtonStyles(int index);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void refreshSceneName();
    void refreshSourceName();
    void renameScene();
    void renameSource();
    void refreshSourceIcon();

private:
    EditorModel *model;

    /* ────────────────────────────────
       FILA 1 — ESCENA
       ──────────────────────────────── */
    QPushButton *sceneIconBtn;
    QComboBox   *sceneCombo;
    QPushButton *sceneFilterPanelBtn = nullptr;   // ← NUEVO

    /* ────────────────────────────────
       FILA 2 — FUENTE
       ──────────────────────────────── */
    QPushButton *sourceIconBtn = nullptr;
    QComboBox   *sourceCombo;

    QPushButton *sourcePropertiesPanelBtn = nullptr;    // ← NUEVO
    QPushButton *sourceTransformPanelBtn = nullptr;    // ← NUEVO
    QPushButton *sourceFiltersPanelBtn = nullptr;    // ← NUEVO

    void setButtonRed(QPushButton *btn);
    void setButtonGreen(QPushButton *btn);

};
