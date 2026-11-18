#pragma once

#include <QMenu>

class NoteEditActions;



class MenuContext : public QMenu
{
public:
    explicit MenuContext(NoteEditActions *acts, QWidget *parent = nullptr);
};
