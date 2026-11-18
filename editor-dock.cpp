#include "editor-dock.hpp"
#include "editor-widget.hpp"
#include "editor-model.hpp"
#include "editor-ui-helpers.hpp"
#include "src/widgets/sign.hpp"
#include <QPainter>
#include "src/filters-files/filters-copy-paste.hpp"
#include "src/filters-files/filters-model.hpp"

#include "src/widgets/header.hpp"
#include "src/widgets/information.hpp"
#include "src/widgets/filters-header.hpp"

#include "src/filters/filters-tab.hpp"
#include "src/scene-filters/scene-filters-tab.hpp"
#include "src/filters-files/filters-renderer.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QTimer>
#include <QCheckBox>
#include <QPropertyAnimation>


extern "C" {
#include <obs.h>
#include <obs-hotkey.h>
#include <obs-frontend-api.h>
}
#include <obs-module.h>
#include <util/config-file.h>

namespace {
class LineBar : public QWidget
{
public:
    explicit LineBar(bool lineAtBottom, QWidget *parent = nullptr)
        : QWidget(parent)
        , bottom(lineAtBottom)
    {
    }

    void setBottomAnchor(QWidget *w, int padX, int padY)
    {
        anchor = w;
        mPadX  = padX;
        mPadY  = padY;
        if (anchor)
            placeAnchor();
    }

protected:
    void resizeEvent(QResizeEvent *e) override
    {
        QWidget::resizeEvent(e);
        placeAnchor();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setPen(QPen(editorui::GetPanelLineColor(), 1));
        const int y = bottom ? height() - 1 : 0;
        p.drawLine(0, y, width(), y);
    }

private:
    void placeAnchor()
    {
        if (!anchor)
            return;
        const int ah = anchor->sizeHint().height();
        if (ah <= 0)
            return;
        const int w = width() - 2 * mPadX;
        anchor->setGeometry(mPadX, height() - ah - mPadY,
                            w > 0 ? w : 0, ah);
    }

    bool bottom;
    QWidget *anchor = nullptr;
    int      mPadX  = 2;
    int      mPadY  = 2;
};
}

EditorDock::EditorDock(EditorModel *model, QWidget *parent)
    : QFrame(parent)
    , model(model)
{
    setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    topBar = new LineBar(true, this);
    topBar->setObjectName("editorTopBar");
    topBar->setFixedHeight(90);

    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(2, 2, 2, 2);
    topLayout->setSpacing(2);

    header = new Header(model, topBar);
    topLayout->addWidget(header);

    header->updatePanelButtonStyles(0);

    layout->addWidget(topBar);

    auto *filtersBarLine = new LineBar(true, this);
    filtersBar = filtersBarLine;
    filtersBar->setObjectName("filtersSubHeaderBar");
    filtersBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *filtersLayout = new QHBoxLayout(filtersBar);
    filtersLayout->setContentsMargins(2, 2, 2, 4);
    filtersLayout->setSpacing(2);

    auto *filtersSizer = new QWidget(filtersBar);
    filtersSizer->setFixedSize(8, 30);
    filtersLayout->addWidget(filtersSizer);

    auto *filtersBox = new QWidget(filtersBar);
    filtersBox->setObjectName("filtersSubHeaderBox");
    auto *boxLayout = new QHBoxLayout(filtersBox);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    boxLayout->setSpacing(0);

    filtersHeader = new FiltersHeader(filtersBox);
    filtersHeader->setObjectName("filtersSubHeader");
    boxLayout->addWidget(filtersHeader);

    filtersBarLine->setBottomAnchor(filtersBox, 2, 4);

    filtersBar->setMaximumHeight(0);
    filtersBar->hide();
    filtersBarAnim = new QPropertyAnimation(filtersBar, "maximumHeight", this);
    filtersBarAnim->setDuration(250);
    filtersBarAnim->setEasingCurve(QEasingCurve::OutSine);
    connect(filtersBarAnim, &QPropertyAnimation::finished, this, [this]() {
        if (!filtersBarShown)
            filtersBar->hide();
    });

    layout->addWidget(filtersBar);

    editorWidget = new EditorWidget(model, this);
    layout->addWidget(editorWidget);

    auto *signBar = new Sign(model, this);
    layout->addWidget(signBar);

    const auto rendererList = editorWidget->findChildren<FiltersRenderer *>();
    for (FiltersRenderer *r : rendererList) {
        connect(r, &FiltersRenderer::popoutCreated,
                this, &EditorDock::registerPopout);

        connect(r, &FiltersRenderer::selectionStateChanged,
                this, &EditorDock::refreshFiltersHeaderStates);
    }

    if (auto *tabWidget = editorWidget->findChild<QTabWidget *>())
        connect(tabWidget, &QTabWidget::currentChanged,
                this, &EditorDock::refreshFiltersHeaderStates);

    connect(header->getSceneFiltersPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showSceneFiltersTab);

    connect(header->getSceneNotesPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showSceneNotesTab);
    connect(header->getSourceNotesPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showSourceNotesTab);

    connect(header->getSourcePanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showSourceTab);

    connect(header->getPropertiesPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showPropertiesTab);

    connect(header->getTransformPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showTransformTab);

    connect(header->getFilterPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showFiltersTab);

    registerHotkeys();

    connect(editorWidget, &EditorWidget::tabChanged,
            header, &Header::updatePanelButtonStyles);

    connect(editorWidget, &EditorWidget::tabChanged,
            this, &EditorDock::onTabChanged);


    connect(filtersHeader, &FiltersHeader::expandCollapseAllRequested,
            this, [this](bool collapsed) {
        if (FiltersRenderer *renderer = currentRenderer())
            renderer->toggleAllExpanded(collapsed);
    });

    connect(filtersHeader, &FiltersHeader::visibilityAllRequested,
            this, [this](bool visible) {
        if (FiltersRenderer *renderer = currentRenderer())
            renderer->setAllVisible(visible);
    });


    connect(filtersHeader, &FiltersHeader::categoryChanged,
            this, [this](FiltersHeader::FilterCategory cat) {

        auto *tabs = editorWidget->findChild<QTabWidget*>();
        if (!tabs)
            return;

        int idx = tabs->currentIndex();
        QWidget *tabWidget = tabs->widget(idx);
        if (!tabWidget)
            return;

        FiltersRenderer *renderer = tabWidget->findChild<FiltersRenderer*>();
        if (!renderer)
            return;

        renderer->setCategory(
            (cat == FiltersHeader::FilterCategory::Effect)
                ? FiltersRenderer::FilterCategory::Effect
                : FiltersRenderer::FilterCategory::Audio
        );
    });

    connect(filtersHeader, &FiltersHeader::addFilterRequested,
        this, [this]() {

        auto *tabs = editorWidget->findChild<QTabWidget*>();
        if (!tabs)
            return;

        int idx = tabs->currentIndex();
        QWidget *tabWidget = tabs->widget(idx);
        if (!tabWidget)
            return;

        FiltersRenderer *renderer = nullptr;

        if (idx == 2) {
            renderer = tabWidget->findChild<FiltersRenderer*>();
        } else if (idx == 3) {
            renderer = tabWidget->findChild<FiltersRenderer*>();
        }

        if (!renderer)
            return;

        FiltersModel *fm = renderer->getModel();
        if (!fm)
            return;

        auto types = fm->getAvailableFilterTypes();
        if (types.empty())
            return;

        std::vector<QString> effectTypes;
        std::vector<QString> audioTypes;

        for (const auto &t : types) {
            QString id = t.toLower();

            bool isAudio =
                id.contains("audio") ||
                id.contains("gain") ||
                id.contains("compressor") ||
                id.contains("limiter") ||
                id.contains("noise") ||
                id.contains("expander") ||
                id.contains("eq") ||
                id.contains("duck") ||
                id.contains("delay") ||
                id.contains("vst") ||
                id.contains("plugin");

            if (isAudio)
                audioTypes.push_back(t);
            else
                effectTypes.push_back(t);
        }

        auto removeDuplicates = [](std::vector<QString> &list) {
            std::vector<QString> unique;
            QSet<QString> seen;

            for (const auto &t : list) {
                QString visible = QString::fromUtf8(
                    obs_source_get_display_name(t.toStdString().c_str())
                );

                if (!seen.contains(visible)) {
                    seen.insert(visible);
                    unique.push_back(t);
                }
            }

            list = unique;
        };

        removeDuplicates(effectTypes);
        removeDuplicates(audioTypes);

        auto sortByDisplayName = [](std::vector<QString> &list) {
            std::sort(list.begin(), list.end(),
                [](const QString &a, const QString &b) {
                    QString da = QString::fromUtf8(
                        obs_source_get_display_name(a.toStdString().c_str()));
                    QString db = QString::fromUtf8(
                        obs_source_get_display_name(b.toStdString().c_str()));
                    return da.localeAwareCompare(db) < 0;
                });
        };

        sortByDisplayName(effectTypes);
        sortByDisplayName(audioTypes);

        FiltersHeader::FilterCategory cat = filtersHeader->activeCategory();

        const std::vector<QString> &filtered =
            (cat == FiltersHeader::FilterCategory::Effect)
                ? effectTypes
                : audioTypes;

        if (filtered.empty())
            return;

        QPushButton *btn = filtersHeader->findChild<QPushButton*>("btnAdd");
        if (!btn)
            return;

        QMenu menu(btn);

        for (const auto &t : filtered) {
            const char *visible = obs_source_get_display_name(t.toStdString().c_str());
            QAction *a = menu.addAction(QString::fromUtf8(visible));

            connect(a, &QAction::triggered, this, [fm, renderer, t]() {
                const char *dn = obs_source_get_display_name(t.toStdString().c_str());
                const QString futureName = dn ? QString::fromUtf8(dn) : t;
                renderer->markExpanded(futureName);

                fm->addFilter(t.toStdString());
            });
        }

        menu.exec(btn->mapToGlobal(QPoint(btn->width() / 2, btn->height())));
    });

    connect(filtersHeader, &FiltersHeader::removeFilterRequested,
            this, [this]() {

        auto *tabs = editorWidget->findChild<QTabWidget*>();
        if (!tabs)
            return;

        int idx = tabs->currentIndex();
        QWidget *tabWidget = tabs->widget(idx);
        if (!tabWidget)
            return;

        FiltersRenderer *renderer = nullptr;

        if (idx == 2) {
            renderer = tabWidget->findChild<FiltersRenderer*>();
        } else if (idx == 3) {
            renderer = tabWidget->findChild<FiltersRenderer*>();
        }

        if (renderer)
            renderer->removeSelectedFilters();
    });

    connect(filtersHeader, &FiltersHeader::copyRequested,
            this, [this]() {
        FiltersRenderer *renderer = currentRenderer();
        if (!renderer)
            return;

        renderer->copyFromHeader();

        auto *tabs = editorWidget->findChild<QTabWidget *>();
        const int idx = tabs ? tabs->currentIndex() : -1;

        if (idx == 2)
            editorui::TriggerNativeAction("actionCopyFilters");
        else if (idx == 3)
            editorui::InvokeNativeMethod("SceneCopyFilters");

        refreshFiltersHeaderStates();
    });

    connect(filtersHeader, &FiltersHeader::pasteRequested,
            this, [this]() {
        FiltersRenderer *renderer = currentRenderer();
        if (!renderer)
            return;

        if (!hasCopiedFilters()) {
            QAction *nativePaste = editorui::FindNativeAction("actionPasteFilters");
            if (nativePaste && nativePaste->isEnabled()) {
                auto *tabs = editorWidget->findChild<QTabWidget *>();
                const int idx = tabs ? tabs->currentIndex() : -1;

                const bool handled = (idx == 3)
                    ? editorui::InvokeNativeMethod("ScenePasteFilters")
                    : editorui::TriggerNativeAction("actionPasteFilters");

                if (handled)
                    return;
            }
        }

        renderer->pasteFromHeader();
    });

    connect(filtersHeader, &FiltersHeader::selectAllRequested,
            this, [this](bool selected) {
        auto *tabs = editorWidget->findChild<QTabWidget*>();
        if (!tabs)
            return;

        int idx = tabs->currentIndex();
        QWidget *tabWidget = tabs->widget(idx);
        if (!tabWidget)
            return;

        FiltersRenderer *renderer = nullptr;

        if (idx == 2) {
            renderer = tabWidget->findChild<FiltersRenderer*>();
        } else if (idx == 3) {
            renderer = tabWidget->findChild<FiltersRenderer*>();
        }

        if (renderer)
            renderer->setAllSelected(selected);
    });

    connect(filtersHeader, &FiltersHeader::popoutAllRequested,
            this, [this]() {
        if (FiltersRenderer *renderer = currentRenderer())
            renderer->popoutAll();
    });

    bottomBar = new LineBar(false, this);
    bottomBar->setObjectName("editorBottomBar");
    bottomBar->setFixedHeight(23);

    auto *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    information = new Information(bottomBar);
    bottomLayout->addWidget(information);

    layout->addWidget(bottomBar);

    if (model) {
        connect(model, &EditorModel::modelUpdated,
                this, &EditorDock::refreshUI);

        connect(model, &EditorModel::transformChanged,
                this, &EditorDock::refreshUI);

        connect(model, &EditorModel::modelUpdated,
                this, &EditorDock::applyDefaultCategoryForSource);

        connect(model, &EditorModel::modelUpdated,
                this, &EditorDock::refreshFiltersHeaderStates);
    }

    auto *footerTimer = new QTimer(this);
    footerTimer->setInterval(500);
    connect(footerTimer, &QTimer::timeout,
            this, &EditorDock::refreshUI);
    footerTimer->start();

    refreshUI();

    refreshFiltersHeaderStates();

}

void EditorDock::applyDefaultCategoryForSource()
{
    if (!model || !filtersHeader)
        return;

    QTabWidget *tabs = editorWidget->findChild<QTabWidget *>();
    const int current = tabs ? tabs->currentIndex() : -1;

    if (current == 3)
        return;

    obs_source_t *src = model->selectedSource();
    const uint32_t flags =
        src ? obs_source_get_output_flags(src) : 0;
    const bool hasAudio = src && (flags & OBS_SOURCE_AUDIO);
    const bool hasVideo = src && (flags & OBS_SOURCE_VIDEO);

    filtersHeader->updateAudioButtonState(hasAudio);

    if (src == lastCategorySource)
        return;
    lastCategorySource = src;

    const auto want =
        (hasAudio && !hasVideo)
            ? FiltersHeader::FilterCategory::Audio
            : FiltersHeader::FilterCategory::Effect;
    if (filtersHeader->activeCategory() != want) {
        filtersHeader->setActiveCategory(want);
        emit filtersHeader->categoryChanged(want);
    }
}

void EditorDock::refreshFiltersHeaderStates()
{
    if (!model || !filtersHeader)
        return;

    auto *tabs = editorWidget->findChild<QTabWidget *>();
    const int idx = tabs ? tabs->currentIndex() : -1;
    const bool sceneTab = (idx == 3);

    const bool hasSource = model->selectedSource() != nullptr;

    bool anyChecked = false;
    if (FiltersRenderer *r = currentRenderer()) {
        const auto checks =
            r->findChildren<QCheckBox *>(QStringLiteral("filterSelectCheck"));
        for (QCheckBox *c : checks) {
            if (c->isChecked()) {
                anyChecked = true;
                break;
            }
        }
    }

    QAction *nativePaste = editorui::FindNativeAction("actionPasteFilters");
    const bool canPaste =
        (sceneTab || hasSource) &&
        (hasCopiedFilters() || (nativePaste && nativePaste->isEnabled()));

    const bool canAdd = sceneTab || hasSource;

    bool effectsOn = false;
    bool audioOn = false;
    if (sceneTab) {
        effectsOn = true;
        audioOn = false;
    } else if (hasSource) {
        const uint32_t flags =
            obs_source_get_output_flags(model->selectedSource());
        effectsOn = (flags & OBS_SOURCE_VIDEO) != 0;
        audioOn = (flags & OBS_SOURCE_AUDIO) != 0;
    }

    filtersHeader->setStatesEnabled(canAdd, anyChecked, anyChecked,
                                    canPaste, effectsOn, audioOn);
}

void EditorDock::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);

    blog(LOG_INFO, "[E33][dock-ancho] dockW=%d dockMinW=%d dockMinHintW=%d",
         width(), minimumWidth(), minimumSizeHint().width());

    for (QObject *o : children()) {
        auto *w = qobject_cast<QWidget *>(o);
        if (!w || !w->isVisible())
            continue;
        const QString name = w->objectName().isEmpty()
            ? QString::fromLatin1(w->metaObject()->className())
            : w->objectName();
        blog(LOG_INFO, "  [E33][dock-ancho]   hijo=%s minHint=%d hint=%d w=%d",
             qPrintable(name),
             w->minimumSizeHint().width(),
             w->sizeHint().width(),
             w->width());
    }
}

void EditorDock::setFiltersBarVisible(bool show)
{
    if (!filtersBar || !filtersBarAnim || show == filtersBarShown)
        return;

    filtersBarShown = show;
    filtersBarAnim->stop();

    filtersBarAnim->setStartValue(filtersBar->maximumHeight());
    filtersBarAnim->setEndValue(
        show ? filtersBar->sizeHint().height() : 0);

    if (show)
        filtersBar->show();
    filtersBarAnim->start();
}

void EditorDock::onTabChanged(int index)
{
    bool show = (index == 2 || index == 3);

    setFiltersBarVisible(show);

    if (index == 3 && filtersHeader) {
        filtersHeader->disableAudioCategory();
    }

    if (index == 2) {
        lastCategorySource = nullptr;
        applyDefaultCategoryForSource();
    }

    refreshFiltersHeaderStates();
}

FiltersRenderer *EditorDock::currentRenderer() const
{
    if (!editorWidget)
        return nullptr;

    auto *tabs = editorWidget->findChild<QTabWidget *>();
    if (!tabs)
        return nullptr;

    QWidget *page = tabs->currentWidget();
    if (!page)
        return nullptr;

    return page->findChild<FiltersRenderer *>();
}

void EditorDock::refreshUI()
{
    if (!model || !information)
        return;

    information->updateInfo(
        model->sourceName(),
        model->sourceType(),
        model->sourceSize(),
        model->scaledSize(),
        model->usedInScenes()
    );
}

void EditorDock::registerPopout(QWidget *w)
{
    openPopouts.append(w);
}

void EditorDock::registerHotkeys()
{
    static const char *const ids[8] = {
        "e33.panel.source", "e33.panel.properties",
        "e33.panel.transform", "e33.panel.filters",
        "e33.panel.scene-filters", "e33.panel.preview",
        "e33.panel.notes-scene", "e33.panel.notes-source"
    };
    static const char *const descKeys[8] = {
        "HotkeyPanelSource", "HotkeyPanelProperties",
        "HotkeyPanelTransform", "HotkeyPanelFilters",
        "HotkeyPanelSceneFilters", "HotkeyPanelPreview",
        "HotkeyPanelNotesScene", "HotkeyPanelNotesSource"
    };

    auto cb = [](void *data, obs_hotkey_id id, obs_hotkey_t *, bool pressed) {
        if (!pressed)
            return;
        static_cast<EditorDock *>(data)->hotkeyDispatch(id);
    };

    for (int i = 0; i < 8; ++i)
        hotkeyIds[i] = obs_hotkey_register_frontend(
            ids[i], obs_module_text(descKeys[i]), cb, this);

    QMetaObject::invokeMethod(this, [this]() {
        config_t *cfg = obs_frontend_get_profile_config();
        if (!cfg)
            return;
        for (int i = 0; i < 8; ++i) {
            const char *json =
                config_get_string(cfg, "Hotkeys", ids[i]);
            if (!json)
                continue;
            obs_data_t *data = obs_data_create_from_json(json);
            if (!data)
                continue;
            obs_data_array_t *arr =
                obs_data_get_array(data, "bindings");
            if (arr) {
                obs_hotkey_load(hotkeyIds[i], arr);
                obs_data_array_release(arr);
            }
            obs_data_release(data);
        }
    }, Qt::QueuedConnection);
}

void EditorDock::unregisterHotkeys()
{
    for (size_t &id : hotkeyIds) {
        if (id) {
            obs_hotkey_unregister(id);
            id = 0;
        }
    }
}

void EditorDock::hotkeyDispatch(size_t id)
{
    QMetaObject::invokeMethod(this, [this, id]() {
        if (!header || !editorWidget || !isVisible())
            return;

        QPushButton *btn = nullptr;
        if      (id == hotkeyIds[0]) btn = header->getSourcePanelButton();
        else if (id == hotkeyIds[1]) btn = header->getPropertiesPanelButton();
        else if (id == hotkeyIds[2]) btn = header->getTransformPanelButton();
        else if (id == hotkeyIds[3]) btn = header->getFilterPanelButton();
        else if (id == hotkeyIds[4]) btn = header->getSceneFiltersPanelButton();
        else if (id == hotkeyIds[5]) btn = header->getPreviewButton();
        else if (id == hotkeyIds[6]) btn = header->getSceneNotesPanelButton();
        else if (id == hotkeyIds[7]) btn = header->getSourceNotesPanelButton();

        if (btn && btn->isEnabled())
            btn->click();
    }, Qt::QueuedConnection);
}

EditorDock::~EditorDock()
{
    unregisterHotkeys();

    for (auto &w : openPopouts) {
        if (w)
            w->close();
    }
}


