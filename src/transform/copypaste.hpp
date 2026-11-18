#pragma once
#include <QWidget>

class QPushButton;

class CopyPasteBox : public QWidget
{
    Q_OBJECT
public:
    explicit CopyPasteBox(QWidget *parent = nullptr);

    void setStatesEnabled(bool copy, bool paste, bool reset);

signals:
    void copyTransform();
    void pasteTransform();
    void resetTransform();

private:
    QPushButton *btnCopy  = nullptr;
    QPushButton *btnPaste = nullptr;
    QPushButton *btnReset = nullptr;
};
