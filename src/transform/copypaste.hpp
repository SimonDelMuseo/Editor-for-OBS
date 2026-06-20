#pragma once
#include <QWidget>

class CopyPasteBox : public QWidget
{
    Q_OBJECT
public:
    explicit CopyPasteBox(QWidget *parent = nullptr);

signals:
    void copyTransform();
    void pasteTransform();
    void resetTransform();
};
