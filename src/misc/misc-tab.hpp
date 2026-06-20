#pragma once
#include <QWidget>

class EditorModel;

class MiscTab : public QWidget
{
    Q_OBJECT
public:
    explicit MiscTab(EditorModel *model, QWidget *parent = nullptr);

private:
    EditorModel *model = nullptr;
};
