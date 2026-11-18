#pragma once

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QWidget>

class QAction;
class QEvent;
class QResizeEvent;
class QPushButton;
class QTextEdit;
class QToolButton;




struct NoteActions {
    QAction *undo = nullptr;
    QAction *redo = nullptr;
    QAction *cut = nullptr;
    QAction *copy = nullptr;
    QAction *paste = nullptr;
    QAction *selectAll = nullptr;
    QAction *font = nullptr;
    QAction *textColor = nullptr;
    QAction *backgroundColor = nullptr;
    QAction *clearFormat = nullptr;
    QList<QAction *> list;
    QAction *indentIncrease = nullptr;
    QAction *indentDecrease = nullptr;
};

class NoteEditActions : public QObject
{
public:
    explicit NoteEditActions(QTextEdit *editor, QObject *parent = nullptr);

    const NoteActions &actions() const { return m_a; }
    QTextEdit *editor() const { return m_editor; }

    void refresh();
    void updateColorIcons();
    void applyThemeIcons();

private:
    void build();
    void registerThemeIcon(QAction *action, const char *themeName);

    QTextEdit *m_editor = nullptr;
    NoteActions m_a;
    QList<QPair<QAction *, QString>> m_themeIcons;
};

class EditNoteBar : public QWidget
{
public:
    explicit EditNoteBar(NoteEditActions *acts, QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    NoteEditActions *m_acts = nullptr;
    QPushButton *m_listBtn = nullptr;
};
