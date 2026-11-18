#include <obs-module.h>

#include "editor-widget.hpp"
#include "editor-model.hpp"

#include "src/source/source-tab.hpp"
#include "src/properties/properties-tab.hpp"
#include "src/filters/filters-tab.hpp"
#include "src/scene-filters/scene-filters-tab.hpp"
#include "src/transform/transform-tab.hpp"
#include "src/notes/notes-tab-scene.hpp"
#include "src/notes/notes-tab-source.hpp"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QTabBar>
#include <QStackedWidget>
#include <QPropertyAnimation>

EditorWidget::EditorWidget(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    tabs = new QTabWidget(this);

    tabs->setStyleSheet(
        QStringLiteral("QTabWidget::pane { border: none; }"));

    tabs->addTab(new SourceTab(model, this),       obs_module_text("SourceTab"));
    tabs->addTab(new PropertiesTab(model, this),   obs_module_text("PropertiesTab"));
    tabs->addTab(new FiltersTab(model, this),      obs_module_text("FiltersTab"));
    tabs->addTab(new SceneFiltersTab(model, this), obs_module_text("SceneFiltersTab"));
    tabs->addTab(new TransformTab(model, this),    obs_module_text("TransformTab"));
    tabs->addTab(new SceneNotesTab(model, this),
                 obs_module_text("SceneNotesTab"));
    tabs->addTab(new SourceNotesTab(model, this),
                 obs_module_text("SourceNotesTab"));
    
    tabs->tabBar()->hide();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabs);

   connect(tabs, &QTabWidget::currentChanged,
        this, &EditorWidget::onTabChanged);

}

void EditorWidget::showSourceTab()
{
    if (tabs)
        tabs->setCurrentIndex(0);
}

void EditorWidget::showPropertiesTab()
{
    if (tabs)
        tabs->setCurrentIndex(1);
}

void EditorWidget::showFiltersTab()
{
    if (tabs)
        tabs->setCurrentIndex(2);
}

void EditorWidget::showSceneFiltersTab()
{
    if (tabs)
        tabs->setCurrentIndex(3);
}

void EditorWidget::showTransformTab()
{
    if (tabs)
        tabs->setCurrentIndex(4);
}

void EditorWidget::showSceneNotesTab()
{
    if (tabs)
        tabs->setCurrentIndex(5);
}

void EditorWidget::showSourceNotesTab()
{
    if (tabs)
        tabs->setCurrentIndex(6);
}

void EditorWidget::onTabChanged(int index)
{
    if (lastTabIndex != index)
        animateTabSwitch(lastTabIndex, index);

    lastTabIndex = index;
    emit tabChanged(index);
}

void EditorWidget::animateTabSwitch(int from, int to)
{
    QStackedWidget *stacked = tabs ? tabs->findChild<QStackedWidget *>() : nullptr;
    QWidget *oldPage = tabs ? tabs->widget(from) : nullptr;
    QWidget *newPage = tabs ? tabs->widget(to) : nullptr;
    if (!stacked || !oldPage || !newPage)
        return;

    for (auto *a : tabSwitchAnims) {
        a->disconnect(this);
        a->stop();
        a->deleteLater();
    }
    tabSwitchAnims.clear();
    if (switchOldPage) {
        switchOldPage->hide();
        switchOldPage = nullptr;
    }
    if (switchNewPage) {
        switchNewPage->move(0, 0);
        switchNewPage = nullptr;
    }
    stacked->clearMask();

    const int w = stacked->width();
    const int h = stacked->height();

    auto buttonPos = [](int idx) {
        switch (idx) {
        case 0: return 0;
        case 1: return 1;
        case 4: return 2;
        case 5:
        case 6: return 4;
        default: return 3;
        }
    };

    const bool touchesScene = (from == 3 || to == 3);
    const bool touchesSourceFilters =
        (from == 2 || to == 2) && !touchesScene;
    const bool touchesNotes =
        (from == 5 || from == 6) && (to == 5 || to == 6);
    const bool goingUp = touchesNotes ? (to == 5)
                        : touchesScene ? (to == 3) : (to == 2);
    const bool goingLeft = !touchesScene && !touchesNotes &&
                           buttonPos(to) < buttonPos(from);

    QPoint outEdge, inEdge;
    if (touchesScene || touchesSourceFilters || touchesNotes) {
        const int out = goingUp ? h : -h;
        outEdge = QPoint(0, out);
        inEdge  = QPoint(0, goingUp ? -h : h);
    } else {
        const int out = goingLeft ? w : -w;
        outEdge = QPoint(out, 0);
        inEdge  = QPoint(goingLeft ? -w : w, 0);
    }

    stacked->setMask(QRegion(stacked->rect()));

    newPage->move(inEdge);
    switchNewPage = newPage;

    oldPage->show();
    oldPage->raise();
    oldPage->move(0, 0);
    switchOldPage = oldPage;

    auto *animOld = new QPropertyAnimation(oldPage, "pos", this);
    animOld->setDuration(250);
    animOld->setStartValue(QPoint(0, 0));
    animOld->setEndValue(outEdge);
    animOld->setEasingCurve(QEasingCurve::OutSine);
    tabSwitchAnims << animOld;

    auto *animNew = new QPropertyAnimation(newPage, "pos", this);
    animNew->setDuration(250);
    animNew->setStartValue(inEdge);
    animNew->setEndValue(QPoint(0, 0));
    animNew->setEasingCurve(QEasingCurve::OutSine);

    connect(animNew, &QPropertyAnimation::finished, this, [this, stacked]() {
        if (switchOldPage) {
            switchOldPage->hide();
            switchOldPage = nullptr;
        }
        if (switchNewPage) {
            switchNewPage->move(0, 0);
            switchNewPage = nullptr;
        }
        stacked->clearMask();
        for (auto *a : tabSwitchAnims)
            a->deleteLater();
        tabSwitchAnims.clear();
    });
    tabSwitchAnims << animNew;

    animOld->start();
    animNew->start();
}