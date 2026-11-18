#pragma once
#include <QWidget>
#include <QMap>

class EditorModel;
class CopyPasteBox;
class QuickBox;
class AlignmentBox;
class EditBox;
class QPropertyAnimation;

class TransformTab : public QWidget
{
    Q_OBJECT
public:
    explicit TransformTab(EditorModel *model, QWidget *parent = nullptr);

private slots:
    void refreshUI();

private:
    EditorModel *model = nullptr;

    void setContainerVisible(QWidget *box, bool wantVisible);
    void startBoxAnim(QWidget *box, int from, int to, bool hideAtEnd);

    CopyPasteBox   *copyPasteBox   = nullptr;
    QuickBox       *quickBox       = nullptr;
    AlignmentBox   *alignmentBox   = nullptr;
    EditBox        *editBox        = nullptr;

    QMap<QWidget *, QPropertyAnimation *> boxAnims;
};
