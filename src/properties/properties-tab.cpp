#include "properties-tab.hpp"
#include "editor-model.hpp"

extern "C" {
#include <obs.h>
#include <obs-properties.h>
#include <obs-frontend-api.h>
}
#include <obs-module.h>

#include <QVBoxLayout>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QLabel>
#include <QMainWindow>
#include <QApplication>

#include <cstring>

#include "properties-view.hpp"

static obs_properties_t *ReloadSourceProperties(void *obj)
{
    return obs_source_properties((obs_source_t *)obj);
}

static void refreshPropertiesViewsFor(obs_source_t *target, bool skipFocused = false)
{
    if (!target)
        return;

    QWidget *focus = QApplication::focusWidget();

    for (OBSPropertiesView *view : AllPropertyViews()) {
        if (!view->IsObject(target))
            continue;
        if (skipFocused && focus && view->isAncestorOf(focus))
            continue;
        view->ReloadProperties();
    }
}

static void properties_undo_cb(const char *data)
{
    blog(LOG_INFO, "[E33][undo] POP-PROP data=%.120s", data ? data : "(null)");
    obs_data_t *settings = obs_data_create_from_json(data);
    if (!settings)
        return;

    const char *uuid = obs_data_get_string(settings, "undo_uuid");
    obs_source_t *source = (uuid && *uuid) ? obs_get_source_by_uuid(uuid) : nullptr;

    if (source) {
        obs_source_reset_settings(source, settings);
        obs_source_update_properties(source);
        refreshPropertiesViewsFor(source);
        obs_source_release(source);
    }

    obs_data_release(settings);

    if (EditorModel *m = EditorModel::instanceForUndo())
        m->modelUpdated();
}

static void UpdateSourceProperties(void *obj, obs_data_t *old_settings, obs_data_t *new_settings)
{
    obs_source_t *source = (obs_source_t *)obj;
    if (!source)
        return;

    obs_data_set_string(old_settings, "undo_uuid", obs_source_get_uuid(source));
    obs_data_set_string(new_settings, "undo_uuid", obs_source_get_uuid(source));

    const char *undo_data = obs_data_get_json(old_settings);
    const char *redo_data = obs_data_get_json(new_settings);

    if (undo_data && redo_data && strcmp(undo_data, redo_data) != 0) {
        blog(LOG_INFO, "[E33][undo] REG-PROP '%s' old=%s",
             obs_source_get_name(source),
             old_settings ? "pre-cambio(BUENA)" : "NULO(basura)");

        QString label = QString(obs_module_text("UndoSourceProperties"))
                            .arg(QString::fromUtf8(obs_source_get_name(source)));

        obs_frontend_add_undo_redo_action(label.toUtf8().constData(),
                                          properties_undo_cb, properties_undo_cb,
                                          undo_data, redo_data, false);
    }

    obs_source_update(source, new_settings);

    refreshPropertiesViewsFor(source, true);
}

PropertiesTab::PropertiesTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    connect(model, &EditorModel::modelUpdated,
            this, &PropertiesTab::refreshUI);

    refreshUI();
}

void PropertiesTab::showNoSourceMessage()
{
    stopContentAnims();

    if (propertiesUI) {
        propertiesUI->deleteLater();
        propertiesUI = nullptr;
    }

    QLayoutItem *item;
    while ((item = mainLayout->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

}

void PropertiesTab::refreshUI()
{
    obs_source_t *source = model->selectedSource();

    if (!source) {
        lastSource = nullptr;
        showNoSourceMessage();
        return;
    }

    if (source != lastSource) {
        lastSource = source;
        buildPropertiesUI(source);
    }
}

void PropertiesTab::buildPropertiesUI(obs_source_t *source)
{
    stopContentAnims();

    OBSPropertiesView *oldView = propertiesUI;

    if (oldView) {
        mainLayout->removeWidget(oldView);
        oldView->setParent(this);
    }

    QLayoutItem *item;
    while ((item = mainLayout->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }
    propertiesUI = nullptr;

    if (!source) {
        showNoSourceMessage();
        return;
    }

    obs_data_t *settings = obs_source_get_settings(source);

    propertiesUI = new OBSPropertiesView(
        OBSData(settings),
        source,
        ReloadSourceProperties,
        UpdateSourceProperties,
        nullptr,
        0
    );

    obs_data_release(settings);

    propertiesUI->setMinimumHeight(150);

    mainLayout->addWidget(propertiesUI);

    if (width() > 0 && isVisible()) {
        mainLayout->activate();
        if (oldView)
            animatePushOut(oldView);
        else
            animateReveal();
    } else if (oldView) {
        oldView->deleteLater();
    }
}

void PropertiesTab::stopContentAnims()
{
    for (QPropertyAnimation *a : contentAnims) {
        a->disconnect(this);
        a->stop();
        a->deleteLater();
    }
    contentAnims.clear();
    clearMask();
    if (outgoingView) {
        outgoingView->deleteLater();
        outgoingView = nullptr;
    }
}

void PropertiesTab::animateReveal()
{
    const QRect g = propertiesUI->geometry();
    mainLayout->removeWidget(propertiesUI);
    propertiesUI->setMinimumHeight(0);
    propertiesUI->setGeometry(0, 0, g.width(), 0);
    propertiesUI->raise();
    propertiesUI->show();

    setMask(QRegion(rect()));

    auto *a = new QPropertyAnimation(propertiesUI, "geometry", this);
    a->setDuration(250);
    a->setStartValue(QRect(0, 0, g.width(), 0));
    a->setEndValue(QRect(0, 0, g.width(), g.height()));
    a->setEasingCurve(QEasingCurve::OutSine);
    connect(a, &QPropertyAnimation::finished, this, [this]() {
        contentAnims.clear();
        clearMask();
        if (propertiesUI) {
            propertiesUI->setMinimumHeight(150);
            mainLayout->addWidget(propertiesUI);
            mainLayout->activate();
        }
    });
    contentAnims << a;
    a->start();
}

void PropertiesTab::animatePushOut(QWidget *oldView)
{
    const QRect gNew = propertiesUI->geometry();
    const QRect gOld = oldView->geometry();

    mainLayout->removeWidget(propertiesUI);
    propertiesUI->setGeometry(gNew);

    propertiesUI->move(gNew.x(), -gNew.height());
    propertiesUI->show();
    oldView->raise();
    oldView->show();
    outgoingView = oldView;

    setMask(QRegion(rect()));

    auto *animOld = new QPropertyAnimation(oldView, "pos", this);
    animOld->setDuration(250);
    animOld->setStartValue(QPoint(gOld.x(), gOld.y()));
    animOld->setEndValue(QPoint(gOld.x(), height()));
    animOld->setEasingCurve(QEasingCurve::OutSine);

    auto *animNew = new QPropertyAnimation(propertiesUI, "pos", this);
    animNew->setDuration(250);
    animNew->setStartValue(QPoint(gNew.x(), -gNew.height()));
    animNew->setEndValue(QPoint(gNew.x(), 0));
    animNew->setEasingCurve(QEasingCurve::OutSine);

    connect(animNew, &QPropertyAnimation::finished, this, [this, oldView]() {
        contentAnims.clear();
        clearMask();
        if (outgoingView == oldView)
            outgoingView = nullptr;
        oldView->deleteLater();
        if (propertiesUI) {
            propertiesUI->move(0, 0);
            mainLayout->addWidget(propertiesUI);
            mainLayout->activate();
        }
    });
    contentAnims << animOld << animNew;
    animOld->start();
    animNew->start();
}
