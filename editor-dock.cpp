/* ────────────────────────────────────────────────
   INCLUDES
   ──────────────────────────────────────────────── */
#include "editor-dock.hpp"
#include "editor-widget.hpp"
#include "editor-model.hpp"
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


extern "C" {
#include <obs.h>
#include <obs-frontend-api.h>
}

/* ────────────────────────────────────────────────
   CONSTRUCTOR — DOCK PADRE EXACTO AL MODELO EXELDRO
   ──────────────────────────────────────────────── */
EditorDock::EditorDock(EditorModel *model, QWidget *parent)
    : QFrame(parent)
    , model(model)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    /* ============================================================
       BARRA SUPERIOR (HEADER PRINCIPAL)
       ============================================================ */
    topBar = new QWidget(this);
    topBar->setObjectName("editorTopBar");
    topBar->setFixedHeight(90);

    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(2, 2, 2, 2);
    topLayout->setSpacing(2);

    Header *header = new Header(model, topBar);
    topLayout->addWidget(header);

    header->updatePanelButtonStyles(0);

    topBar->setStyleSheet(
        "#editorTopBar {"
        " border-bottom: 1px solid rgba(255,255,255,40);"
        "}"
    );

    layout->addWidget(topBar);

    /* ============================================================
       ➕ SUBHEADER DE FILTROS (OCULTO POR DEFECTO)
       ============================================================ */
    auto *filtersBar = new QWidget(this);
    filtersBar->setObjectName("filtersSubHeaderBar");

    auto *filtersLayout = new QHBoxLayout(filtersBar);
    filtersLayout->setContentsMargins(2, 2, 2, 2); // mismo padding que el header
    filtersLayout->setSpacing(2);

    filtersHeader = new FiltersHeader(filtersBar);
    filtersHeader->setObjectName("filtersSubHeader");
    filtersHeader->setVisible(false);

    // ⬅️ Mueve el borde al contenedor, igual que en el header principal
    filtersBar->setStyleSheet(
        "#filtersSubHeaderBar {"
        " border-bottom: 1px solid rgba(255,255,255,40);"
        "}"
    );

    filtersLayout->addWidget(filtersHeader);

    layout->addWidget(filtersBar);

    /* ============================================================
       WIDGET CENTRAL (EDITOR)
       ============================================================ */
    editorWidget = new EditorWidget(model, this);
    layout->addWidget(editorWidget);

    /* ============================================================
       CONEXIONES DEL HEADER PRINCIPAL
       ============================================================ */
    connect(header->getSceneFiltersPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showSceneFiltersTab);

    connect(header->getSourcePanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showSourceTab);

    connect(header->getPropertiesPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showPropertiesTab);

    connect(header->getTransformPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showTransformTab);

    connect(header->getFilterPanelButton(), &QPushButton::clicked,
            editorWidget, &EditorWidget::showFiltersTab);

    connect(editorWidget, &EditorWidget::tabChanged,
            header, &Header::updatePanelButtonStyles);

    /* ============================================================
       ➕ CONEXIÓN — MOSTRAR/OCULTAR SUBHEADER SEGÚN TAB
       ============================================================ */
    connect(editorWidget, &EditorWidget::tabChanged,
            this, &EditorDock::onTabChanged);

    /* ============================================================
       ➕ CONEXIONES — EXPANDIR / COLAPSAR TODOS DESDE EL HEADER
       ============================================================ */

    // Expandir / colapsar todos los filtros del tab actual (fuente o escena)
    connect(filtersHeader, &FiltersHeader::expandCollapseAllRequested,
            this, [this](bool collapsed) {
        auto *tabs = editorWidget->findChild<QTabWidget*>();
        if (!tabs)
            return;

        int idx = tabs->currentIndex();
        QWidget *tabWidget = tabs->widget(idx);
        if (!tabWidget)
            return;

        FiltersRenderer *renderer = nullptr;

        if (idx == 2) { // FiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        } else if (idx == 3) { // SceneFiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        }

        if (renderer)
            connect(renderer, &FiltersRenderer::popoutCreated,
                    this, &EditorDock::registerPopout);

            renderer->toggleAllExpanded(collapsed);
    });

    /* ============================================================
       ➕ CONEXIONES — MOSTRAR / OCULTAR TODOS DESDE EL HEADER
       ============================================================ */  
    connect(filtersHeader, &FiltersHeader::visibilityAllRequested,
            this, [this](bool visible) {
        auto *tabs = editorWidget->findChild<QTabWidget*>();
        if (!tabs)
            return;

        int idx = tabs->currentIndex();
        QWidget *tabWidget = tabs->widget(idx);
        if (!tabWidget)
            return;

        FiltersRenderer *renderer = nullptr;

        if (idx == 2) { // FiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        } else if (idx == 3) { // SceneFiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        }

        if (renderer)
            connect(renderer, &FiltersRenderer::popoutCreated,
                    this, &EditorDock::registerPopout);
        
            renderer->setAllVisible(visible);
    });

    /* ============================================================
            ➕ BOTÓN “AÑADIR FILTRO” — MENÚ DE TIPOS DISPONIBLES
        ============================================================ */

    // 🔥 NUEVO: conectar el cambio de categoría al renderer
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

        if (idx == 2) { // FiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        } else if (idx == 3) { // SceneFiltersTab
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

        // ============================================================
        // 🔥 CLASIFICACIÓN PERFECTA SIN INSTANCIAS
        // ============================================================
        std::vector<QString> effectTypes;
        std::vector<QString> audioTypes;

        for (const auto &t : types) {
            QString id = t.toLower();

            // ✔ EXACTO: clasificación real de OBS v32
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
                id.contains("vst") ||      // VST siempre audio
                id.contains("plugin");     // plugins externos → audio

            if (isAudio)
                audioTypes.push_back(t);
            else
                effectTypes.push_back(t);
        }

        // ============================================================
        // 🔥 ELIMINAR DUPLICADOS POR DISPLAY NAME
        // ============================================================
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

        // ============================================================
        // 🔥 ORDEN ALFABÉTICO REAL POR DISPLAY NAME
        // ============================================================
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

        // ============================================================
        // 🔥 ELEGIR LISTA SEGÚN CATEGORÍA
        // ============================================================
        FiltersHeader::FilterCategory cat = filtersHeader->activeCategory();

        const std::vector<QString> &filtered =
            (cat == FiltersHeader::FilterCategory::Effect)
                ? effectTypes
                : audioTypes;

        if (filtered.empty())
            return;

        // ============================================================
        // 🔥 CREAR MENÚ
        // ============================================================
        QPushButton *btn = filtersHeader->findChild<QPushButton*>("btnAdd");
        if (!btn)
            return;

        QMenu menu(btn);

        for (const auto &t : filtered) {
            const char *visible = obs_source_get_display_name(t.toStdString().c_str());
            QAction *a = menu.addAction(QString::fromUtf8(visible));

            connect(a, &QAction::triggered, this, [fm, t]() {
                fm->addFilter(t.toStdString());
            });
        }

        menu.exec(btn->mapToGlobal(QPoint(btn->width() / 2, btn->height())));
    });

    /* ============================================================
        ➕ CONEXIONES — ELIMINAR FILTROS SELECCIONADOS
        ============================================================ */
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

        if (idx == 2) { // FiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        } else if (idx == 3) { // SceneFiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        }

        if (renderer)
            renderer->removeSelectedFilters();
    });

    /* ============================================================
    ➕ CONEXIONES — COPIAR / PEGAR DESDE EL HEADER
    ============================================================ */
    connect(filtersHeader, &FiltersHeader::copyRequested,
            this, [this]() {

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

        renderer->copyFromHeader();
    });

    connect(filtersHeader, &FiltersHeader::pasteRequested,
            this, [this]() {

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

        renderer->pasteFromHeader();
    });

    /* ============================================================
       ➕ CONEXIONES — SELECCIONAR / DESELECCIONAR TODOS
       ============================================================ */
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

        if (idx == 2) { // FiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        } else if (idx == 3) { // SceneFiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        }

        if (renderer)
            renderer->setAllSelected(selected);
    });

    /* ============================================================
       ➕ CONEXIONES — POPOUT ALL (abrir todos los filtros)
       ============================================================ */
    connect(filtersHeader, &FiltersHeader::popoutAllRequested,
            this, [this]() {
        auto *tabs = editorWidget->findChild<QTabWidget*>();
        if (!tabs)
            return;

        int idx = tabs->currentIndex();
        QWidget *tabWidget = tabs->widget(idx);
        if (!tabWidget)
            return;

        FiltersRenderer *renderer = nullptr;

        if (idx == 2) { // FiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        } else if (idx == 3) { // SceneFiltersTab
            renderer = tabWidget->findChild<FiltersRenderer*>();
        }

        if (renderer)
            connect(renderer, &FiltersRenderer::popoutCreated,
                    this, &EditorDock::registerPopout);

            renderer->popoutAll();
    });

    /* ============================================================
       BARRA INFERIOR (INFO GENERAL)
       ============================================================ */
    bottomBar = new QWidget(this);
    bottomBar->setObjectName("editorBottomBar");
    bottomBar->setFixedHeight(23);

    auto *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    information = new Information(bottomBar);
    bottomLayout->addWidget(information);

    bottomBar->setStyleSheet(
        "#editorBottomBar {"
        " border-top: 1px solid rgba(255,255,255,40);"
        "}"
    );

    layout->addWidget(bottomBar);

    /* ============================================================
    CONEXIÓN — ACTUALIZAR FOOTER CUANDO CAMBIA EL MODELO
    ============================================================ */
    if (model) {
        connect(model, &EditorModel::modelUpdated,
                this, &EditorDock::refreshUI);

        // 🔥 ÚNICO CAMBIO: activar/desactivar botón AudioFilters
        connect(model, &EditorModel::modelUpdated,
        this, [this, model]() {

            // Obtener el QTabWidget real
            QTabWidget *tabs = editorWidget->findChild<QTabWidget*>();
            if (!tabs)
                return;

            int current = tabs->currentIndex();

            // Si estamos en pestaña de filtros de escena → NO tocar AudioFilters
            if (current == 3) {
                return;
            }

            obs_source_t *src = model->selectedSource();
            bool hasAudio = src && (obs_source_get_output_flags(src) & OBS_SOURCE_AUDIO);

            // Actualizar estado del botón Audio
            filtersHeader->updateAudioButtonState(hasAudio);

            // Si estábamos en AudioFilters y la fuente NO tiene audio → cambiar a EffectFilters
            if (!hasAudio &&
                filtersHeader->activeCategory() == FiltersHeader::FilterCategory::Audio) {

                filtersHeader->setActiveCategory(FiltersHeader::FilterCategory::Effect);
                emit filtersHeader->categoryChanged(FiltersHeader::FilterCategory::Effect);
            }
        });
    }

    refreshUI();

}

/* ────────────────────────────────────────────────
   MOSTRAR / OCULTAR SUBHEADER SEGÚN PESTAÑA ACTIVA
   ──────────────────────────────────────────────── */
void EditorDock::onTabChanged(int index)
{
    bool show = (index == 2 || index == 3);

    if (filtersHeader)
        filtersHeader->setVisible(show);

    // 🔥 Si estamos en filtros de escena → AudioFilters siempre apagado
    if (index == 3 && filtersHeader) {
        filtersHeader->disableAudioCategory();
    }
}

/* ────────────────────────────────────────────────
   REFRESCAR FOOTER (INFORMACIÓN GLOBAL)
   ──────────────────────────────────────────────── */
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

EditorDock::~EditorDock()
{
    for (auto &w : openPopouts) {
        if (w)
            w->close();   // dispara closeEvent → detiene timers → NO CRASH
    }
}
