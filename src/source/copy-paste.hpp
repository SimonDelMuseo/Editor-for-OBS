#pragma once

#include <QWidget>

class QPushButton;
class Box;
class EditorModel;

class CopyPaste : public QWidget
{
    Q_OBJECT

public:
    explicit CopyPaste(QWidget *parent = nullptr);

    void setModel(EditorModel *m) { model = m; }

private:
    EditorModel *model = nullptr;

    QPushButton *btnCopy = nullptr;
    QPushButton *btnPasteRef = nullptr;
    QPushButton *btnPasteDup = nullptr;
};
