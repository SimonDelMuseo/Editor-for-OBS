#pragma once
#include <QWidget>

class EditPosition;
class EditBounds;
class EditCrop;

class EditBox : public QWidget
{
    Q_OBJECT
public:
    explicit EditBox(QWidget *parent = nullptr);

private:
    EditPosition *positionBox = nullptr;
    EditBounds   *boundsBox   = nullptr;
    EditCrop     *cropBox     = nullptr;
};
