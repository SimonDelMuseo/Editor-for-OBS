#pragma once
#include <QFrame>

extern "C" {
#include <obs-frontend-api.h>
}

class QComboBox;
class QLineEdit;
class QPushButton;
class QVariantAnimation;
class QTimer;
class EditorModel;
class PreviewSource;

class Header : public QFrame
{
    Q_OBJECT

public:
    explicit Header(EditorModel *model, QWidget *parent = nullptr);
    ~Header() override;

    QPushButton *getSourcePanelButton() const { return sourceIconBtn; }
    QPushButton *getPropertiesPanelButton() const { return sourcePropertiesPanelBtn; }
    QPushButton *getFilterPanelButton() const { return sourceFiltersPanelBtn; }
    QPushButton *getSourceNotesPanelButton() const { return sourceNotesBtn; }
    QPushButton *getSceneFiltersPanelButton() const { return sceneFilterPanelBtn; }
    QPushButton *getSceneNotesPanelButton() const { return sceneNotesBtn; }
    QPushButton *getTransformPanelButton() const { return sourceTransformPanelBtn; }
    QPushButton *getPreviewButton() const { return previewBtn; }

    void updatePanelButtonStyles(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void refreshSceneName();
    void refreshSceneList();
    void refreshSourceCount();
    void onSceneComboActivated(int index);
    void refreshSourceName();
    void renameScene();
    void renameSource();
    void refreshSourceIcon();
    void refreshPropertiesBtnState();

private:
    EditorModel *model;

    QPushButton *sceneIconBtn;
    QComboBox   *sceneCombo;
    QPushButton *sceneFilterPanelBtn = nullptr;
    QPushButton *sceneNotesBtn = nullptr;
    QLineEdit   *sourceCountEdit = nullptr;

    QPushButton *sourceIconBtn = nullptr;
    QComboBox   *sourceCombo;

    QPushButton *sourcePropertiesPanelBtn = nullptr;
    QPushButton *sourceTransformPanelBtn = nullptr;
    QPushButton *sourceFiltersPanelBtn = nullptr;
    QPushButton *sourceNotesBtn = nullptr;

    bool editingSceneName = false;

    int activePanelIndex = 0;

    void populateSceneList();
    void setButtonRed(QPushButton *btn);
    void setButtonGreen(QPushButton *btn);

    void refreshPreviewBtn();
    void applyPreviewBtnIcon();
    QPushButton *previewBtn = nullptr;
    PreviewSource *previewWin = nullptr;
    QVariantAnimation *previewBlink = nullptr;
    QTimer *previewAutoClose = nullptr;
    int previewState = 0;

    static void frontend_cb(enum obs_frontend_event event, void *data);
};
