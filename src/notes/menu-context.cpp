#include "menu-context.hpp"
#include "edit-note-bar.hpp"

#include <obs-module.h>

#include <QTextEdit>

MenuContext::MenuContext(NoteEditActions *acts, QWidget *parent)
    : QMenu(parent)
{
    const NoteActions &a = acts->actions();

    addAction(a.undo);
    addAction(a.redo);
    addSeparator();
    addAction(a.cut);
    addAction(a.copy);
    addAction(a.paste);
    addSeparator();
    addAction(a.selectAll);

    if (!acts->editor()->isReadOnly()) {
        addSeparator();
        addAction(a.font);
        addAction(a.textColor);
        addAction(a.backgroundColor);

        QMenu *listMenu = addMenu(
            QString::fromUtf8(obs_module_text("NotesList")));
        for (QAction *la : a.list)
            listMenu->addAction(la);
        listMenu->addSeparator();
        listMenu->addAction(a.indentIncrease);
        listMenu->addAction(a.indentDecrease);

        addSeparator();
        addAction(a.clearFormat);
    }

    connect(this, &QMenu::aboutToShow, this, [acts]() { acts->refresh(); });
}
