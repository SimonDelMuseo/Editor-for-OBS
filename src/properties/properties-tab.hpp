#pragma once

#include <QWidget>
#include <QPointer>

extern "C" {
#include <obs.h>
}

class EditorModel;
class QVBoxLayout;
class OBSPropertiesView;
class QPropertyAnimation;

class PropertiesTab : public QWidget
{
    Q_OBJECT
public:
    explicit PropertiesTab(EditorModel *model, QWidget *parent = nullptr);

private slots:
    void refreshUI();

private:
    void buildPropertiesUI(obs_source_t *source);
    void showNoSourceMessage();

    void animateReveal();
    void animatePushOut(QWidget *oldView);
    void stopContentAnims();

private:
    EditorModel        *model        = nullptr;
    QVBoxLayout        *mainLayout   = nullptr;
    OBSPropertiesView  *propertiesUI = nullptr;

    obs_source_t       *lastSource   = nullptr;

    QList<QPropertyAnimation *> contentAnims;
    QPointer<QWidget>            outgoingView;
};
