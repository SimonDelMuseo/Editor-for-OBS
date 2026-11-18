#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QWidget>

class EditorModel;
class QTabBar;
class QPushButton;
class QLineEdit;
class QTextEdit;
class QTimer;
class QMouseEvent;
class NoteEditActions;
class EditNoteBar;



class SceneNotesTab : public QWidget
{
    Q_OBJECT

public:
    explicit SceneNotesTab(EditorModel *model, QWidget *parent = nullptr);
    ~SceneNotesTab() override;

private slots:
    void refresh();

private:
    EditorModel *model = nullptr;
    QTextEdit *editor = nullptr;

    NoteEditActions *editActions = nullptr;
    EditNoteBar *editBar = nullptr;

    QString currentUuid;
    bool loading = false;
    bool refreshed = false;

    struct NoteTab { QString name; QString text; };
    QVector<NoteTab> tabs;
    QTabBar *bar = nullptr;
    QPushButton *addBtn = nullptr;
    QPushButton *delBtn = nullptr;
    QLineEdit *renameEdit = nullptr;

    QJsonObject data;
    bool uiDirty = false;
    bool fileDirty = false;
    QTimer *saveTimer = nullptr;

    QString filePath() const;
    void load();
    void loadEntity(const QString &uuid);
    QJsonObject serializeEntity() const;
    QString defaultTabName() const;
    void scheduleSave();
    void flush();
    QString uuidForCurrent() const;

    void onBarCurrentChanged(int index);
    void onTabCloseRequested(int index);
    void onAddTab();
    void startRename(int index);
    void commitRename(int index, const QString &original);
    void cancelRename();
    void clearBar();
    void mousePressEvent(QMouseEvent *event) override;
};
