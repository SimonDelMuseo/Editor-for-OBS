#pragma once
#include <QWidget>

#define E33_VERSION "2.0.0"

class QLabel;
class QPropertyAnimation;
class EditorModel;

class Sign : public QWidget
{
    Q_OBJECT

public:
    explicit Sign(EditorModel *model, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void refreshState();

private:
    void animateTo(bool wantShow);

    EditorModel *model     = nullptr;
    QLabel *label          = nullptr;
    QPropertyAnimation *anim = nullptr;
    bool shown             = false;
    bool firstRefresh      = true;
};