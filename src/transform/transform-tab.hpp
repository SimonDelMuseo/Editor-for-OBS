#pragma once
#include <QWidget>

class EditorModel;
class CopyPasteBox;
class QuickBox;
class AlignmentBox;
class EditBox;

class TransformTab : public QWidget
{
    Q_OBJECT
public:
    explicit TransformTab(EditorModel *model, QWidget *parent = nullptr);

private slots:
    void refreshUI();

private:
    EditorModel *model = nullptr;

    /* Subcajas */
    CopyPasteBox   *copyPasteBox   = nullptr;
    QuickBox       *quickBox       = nullptr;
    AlignmentBox   *alignmentBox   = nullptr;
    EditBox        *editBox        = nullptr;
};
