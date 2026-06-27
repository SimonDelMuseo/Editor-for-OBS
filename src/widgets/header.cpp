#include "header.hpp"
#include "editor-model.hpp"
#include "editor-ui-helpers.hpp"

#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <obs-frontend-api.h>

static bool s_editingSourceName = false;

/* ────────────────────────────────────────────────
   CONSTRUCTOR — MONTAJE DEL HEADER (ESCENA + FUENTE)
   ──────────────────────────────────────────────── */
Header::Header(EditorModel *model, QWidget *parent)
    : QFrame(parent)
    , model(model)
{
    setObjectName("E33_Header");
    
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(4);

    /* ──────────────────────────────────────────────────
       FILA 1 — ICONO ESCENA + COMBO + BOTÓN PANEL
       ──────────────────────────────────────────────── */
    auto *sceneRow = new QHBoxLayout();
    sceneRow->setSpacing(6);

    sceneIconBtn = new QPushButton(this);
    sceneIconBtn->setFlat(true);
    sceneIconBtn->setFixedSize(26, 26);
    sceneIconBtn->setFocusPolicy(Qt::NoFocus);
    sceneIconBtn->setObjectName("sceneIconBtn");

    /* QUITAR SOLO EL HOVER */
    sceneIconBtn->setStyleSheet(
        "#sceneIconBtn:hover { background: transparent; }"
    );

    /* DESACTIVAR CLIC SIN DESHABILITAR (para no oscurecer) */
    sceneIconBtn->setAttribute(Qt::WA_TransparentForMouseEvents);
    sceneIconBtn->setCursor(Qt::ArrowCursor);

    sceneCombo = new QComboBox(this);
    sceneCombo->setEditable(true);
    sceneCombo->setInsertPolicy(QComboBox::NoInsert);
    sceneCombo->lineEdit()->setFocusPolicy(Qt::StrongFocus);

    sceneCombo->setStyleSheet(
        "QComboBox::drop-down { width: 0px; border: none; }"
        "QComboBox::down-arrow { image: none; }"
    );

    /* ────────────────────────────────────────────────
    ESTILO COMPLETO PARA LOS QLINEEDIT INTERNOS
    (hover ≠ edición, solo focus marca edición)
    ──────────────────────────────────────────────── */
    sceneCombo->lineEdit()->setStyleSheet(
        "QLineEdit { "
        "  background: transparent; "
        "  border: 1px solid transparent; "
        "} "
        "QLineEdit:hover { "
        "  background: transparent; "
        "  border: 1px solid transparent; "
        "} "
        "QLineEdit:focus { "
        "  background: #202020; "
        "  border: 1px solid #888888; "
        "}"
    );

    /* BOTÓN PANEL ESCENA */
    sceneFilterPanelBtn = new QPushButton(this);
    sceneFilterPanelBtn->setFlat(true);
    sceneFilterPanelBtn->setFixedSize(26, 26);
    sceneFilterPanelBtn->setObjectName("sourceFiltersButton");
    sceneFilterPanelBtn->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        sceneFilterPanelBtn,
        editorui::GetIconPath("Filter.svg").c_str()
    );

    sceneRow->addWidget(sceneIconBtn);
    sceneRow->addWidget(sceneCombo);
    sceneRow->addWidget(sceneFilterPanelBtn);

    /* ────────────────────────────────────────────────────
       FILA 2 — ICONO FUENTE + COMBO + 3 BOTONES PANEL
       ──────────────────────────────────────────────── */
    auto *sourceRow = new QHBoxLayout();
    sourceRow->setSpacing(6);

    sourceIconBtn = new QPushButton(this);
    sourceIconBtn->setFlat(true);
    sourceIconBtn->setFixedSize(26, 26);
    sourceIconBtn->setFocusPolicy(Qt::NoFocus);
    sourceIconBtn->setObjectName("sourceIconBtn");

    sourceCombo = new QComboBox(this);
    sourceCombo->setEditable(true);
    sourceCombo->setInsertPolicy(QComboBox::NoInsert);
    sourceCombo->lineEdit()->setFocusPolicy(Qt::StrongFocus);

    sourceCombo->setStyleSheet(
        "QComboBox::drop-down { width: 0px; border: none; }"
        "QComboBox::down-arrow { image: none; }"
    );

    sourceCombo->lineEdit()->setStyleSheet(
        "QLineEdit { "
        "  background: transparent; "
        "  border: 1px solid transparent; "
        "} "
        "QLineEdit:hover { "
        "  background: transparent; "
        "  border: 1px solid transparent; "
        "} "
        "QLineEdit:focus { "
        "  background: #202020; "
        "  border: 1px solid #888888; "
        "}"
    );

    /* TRES BOTONES PANEL FUENTE */
    sourcePropertiesPanelBtn = new QPushButton(this);
    sourcePropertiesPanelBtn->setFlat(true);
    sourcePropertiesPanelBtn->setFixedSize(26, 26);
    sourcePropertiesPanelBtn->setObjectName("sourcePropertiesButton");
    sourcePropertiesPanelBtn->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        sourcePropertiesPanelBtn,
        editorui::GetIconPath("Properties.svg").c_str()
    );

    sourceTransformPanelBtn = new QPushButton(this);
    sourceTransformPanelBtn->setFlat(true);
    sourceTransformPanelBtn->setFixedSize(26, 26);
    sourceTransformPanelBtn->setObjectName("transformButton");
    sourceTransformPanelBtn->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        sourceTransformPanelBtn,
        editorui::GetIconPath("Transform.svg").c_str()
    );

    sourceFiltersPanelBtn = new QPushButton(this);
    sourceFiltersPanelBtn->setFlat(true);
    sourceFiltersPanelBtn->setFixedSize(26, 26);
    sourceFiltersPanelBtn->setObjectName("sourceFiltersButton");
    sourceFiltersPanelBtn->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        sourceFiltersPanelBtn,
        editorui::GetIconPath("Filter.svg").c_str()
    );
    
    sourceRow->addWidget(sourceIconBtn);
    sourceRow->addWidget(sourceCombo);
    sourceRow->addWidget(sourcePropertiesPanelBtn);
    sourceRow->addWidget(sourceTransformPanelBtn);
    sourceRow->addWidget(sourceFiltersPanelBtn);

    /* ⬅️ ESTAS DOS LÍNEAS SON LAS QUE FALTABAN */
    mainLayout->addLayout(sceneRow);
    mainLayout->addLayout(sourceRow);

    /* ────────────────────────────────────────────────
       CONEXIONES
       ──────────────────────────────────────────────── */
    connect(model, &EditorModel::sceneChanged,
            this,  &Header::refreshSceneName);

    connect(model, &EditorModel::modelUpdated,
            this,  &Header::refreshSourceName);

    connect(model, &EditorModel::modelUpdated,
            this,  &Header::refreshSourceIcon);

    connect(sceneCombo->lineEdit(), &QLineEdit::editingFinished,
            this, &Header::renameScene);

    connect(sourceCombo->lineEdit(), &QLineEdit::textEdited,
            this, [](const QString &) {
                s_editingSourceName = true;
            });

    connect(sourceCombo->lineEdit(), &QLineEdit::editingFinished,
            this, [this]() {
                s_editingSourceName = false;
                renameSource();
            });

    /* ────────────────────────────────────────────────
       CALLBACK PARA ICONO DE ESCENA + ICONO DE FUENTE
       ──────────────────────────────────────────────── */
    obs_frontend_add_event_callback([](obs_frontend_event event, void *data) {
        if (event != OBS_FRONTEND_EVENT_FINISHED_LOADING &&
            event != OBS_FRONTEND_EVENT_THEME_CHANGED)
            return;

        Header *self = static_cast<Header *>(data);

        QWidget *main = static_cast<QWidget *>(obs_frontend_get_main_window());
        if (!main)
            return;

        /* ICONO DE ESCENA */
        {
            QVariant v = main->property("sceneIcon");
            if (v.isValid()) {
                QIcon icon = v.value<QIcon>();
                if (!icon.isNull()) {
                    self->sceneIconBtn->setIcon(icon);
                    self->sceneIconBtn->setIconSize(QSize(18, 18));
                }
            }
        }

        /* ICONO DE FUENTE */
        self->refreshSourceIcon();

    }, this);

    /* ────────────────────────────────────────────────
       INICIALIZACIÓN
       ──────────────────────────────────────────────── */
    refreshSceneName();
    refreshSourceName();
}


/* ────────────────────────────────────────────────
   REFRESCAR ICONO DE FUENTE (Q_PROPERTY)
   ──────────────────────────────────────────────── */
void Header::refreshSourceIcon()
{
    obs_source_t *src = model->currentSourcePtr();
    if (!src) {
        sourceIconBtn->setIcon(QIcon());
        return;
    }

    QWidget *main = static_cast<QWidget *>(obs_frontend_get_main_window());
    if (!main)
        return;

    /* ────────────────────────────────────────────────
       DETECCIÓN POR ID — ESCENA / GRUPO
       (OBS NO USA ICON_TYPE PARA ESTOS CASOS)
       ──────────────────────────────────────────────── */
    const char *id = obs_source_get_unversioned_id(src);
    if (!id) {
        sourceIconBtn->setIcon(QIcon());
        return;
    }

    /* ESCENA COMO FUENTE */
    if (strcmp(id, "scene") == 0) {
        QVariant v = main->property("sceneIcon");
        if (v.isValid()) {
            QIcon icon = v.value<QIcon>();
            if (!icon.isNull()) {
                sourceIconBtn->setIcon(icon);
                sourceIconBtn->setIconSize(QSize(18, 18));
            }
        }
        return;
    }

    /* GRUPO COMO FUENTE */
    if (strcmp(id, "group") == 0) {
        QVariant v = main->property("groupIcon");
        if (v.isValid()) {
            QIcon icon = v.value<QIcon>();
            if (!icon.isNull()) {
                sourceIconBtn->setIcon(icon);
                sourceIconBtn->setIconSize(QSize(18, 18));
            }
        }
        return;
    }

    /* ────────────────────────────────────────────────
       ICONOS NORMALES SEGÚN TIPO DE FUENTE
       ──────────────────────────────────────────────── */
    obs_icon_type type = obs_source_get_icon_type(id);
    const char *prop = "defaultIcon";

    switch (type) {
    case OBS_ICON_TYPE_IMAGE:                  prop = "imageIcon"; break;
    case OBS_ICON_TYPE_COLOR:                  prop = "colorIcon"; break;
    case OBS_ICON_TYPE_SLIDESHOW:              prop = "slideshowIcon"; break;
    case OBS_ICON_TYPE_AUDIO_INPUT:            prop = "audioInputIcon"; break;
    case OBS_ICON_TYPE_AUDIO_OUTPUT:           prop = "audioOutputIcon"; break;
    case OBS_ICON_TYPE_DESKTOP_CAPTURE:        prop = "desktopCapIcon"; break;
    case OBS_ICON_TYPE_WINDOW_CAPTURE:         prop = "windowCapIcon"; break;
    case OBS_ICON_TYPE_GAME_CAPTURE:           prop = "gameCapIcon"; break;
    case OBS_ICON_TYPE_CAMERA:                 prop = "cameraIcon"; break;
    case OBS_ICON_TYPE_TEXT:                   prop = "textIcon"; break;
    case OBS_ICON_TYPE_MEDIA:                  prop = "mediaIcon"; break;
    case OBS_ICON_TYPE_BROWSER:                prop = "browserIcon"; break;
    case OBS_ICON_TYPE_PROCESS_AUDIO_OUTPUT:   prop = "audioProcessOutputIcon"; break;
    default:                                   prop = "defaultIcon"; break;
    }

    QVariant v = main->property(prop);
    if (v.isValid()) {
        QIcon icon = v.value<QIcon>();
        if (!icon.isNull()) {
            sourceIconBtn->setIcon(icon);
            sourceIconBtn->setIconSize(QSize(18, 18));
        }
    }
}



/* ────────────────────────────────────────────────
   SIN PINTADO PERSONALIZADO
   ──────────────────────────────────────────────── */
void Header::paintEvent(QPaintEvent *e)
{
    QFrame::paintEvent(e);
}

/* ────────────────────────────────────────────────
   REFRESCAR NOMBRE DE ESCENA
   ──────────────────────────────────────────────── */
void Header::refreshSceneName()
{
    sceneCombo->blockSignals(true);

    sceneCombo->clear();
    sceneCombo->addItem("");
    sceneCombo->lineEdit()->setText(model->sceneName());

    sceneCombo->blockSignals(false);
}

/* ────────────────────────────────────────────────
   REFRESCAR NOMBRE DE FUENTE
   ──────────────────────────────────────────────── */
void Header::refreshSourceName()
{
    if (s_editingSourceName)
        return;

    sourceCombo->blockSignals(true);

    sourceCombo->clear();
    sourceCombo->addItem("");
    sourceCombo->lineEdit()->setText(model->sourceName());

    sourceCombo->blockSignals(false);
}

/* ────────────────────────────────────────────────
   RENOMBRAR ESCENA
   ──────────────────────────────────────────────── */
void Header::renameScene()
{
    obs_source_t *sceneSrc = model->sceneSourcePtr();
    if (!sceneSrc)
        return;

    QString newName = sceneCombo->currentText();
    obs_source_set_name(sceneSrc, newName.toUtf8().constData());

    // 🔥 NUEVO: salir del modo edición inmediatamente
    sceneCombo->clearFocus();
    sceneCombo->lineEdit()->clearFocus();
}

/* ────────────────────────────────────────────────
   RENOMBRAR FUENTE
   ──────────────────────────────────────────────── */
void Header::renameSource()
{
    obs_source_t *src = model->currentSourcePtr();
    if (!src)
        return;

    QString newName = sourceCombo->currentText();
    obs_source_set_name(src, newName.toUtf8().constData());

    // 🔥 NUEVO: salir del modo edición inmediatamente
    sourceCombo->clearFocus();
    sourceCombo->lineEdit()->clearFocus();
}

/* ────────────────────────────────────────────────
   BOTONES BORDES ROJOS
   ──────────────────────────────────────────────── */
void Header::setButtonRed(QPushButton *btn)
{
    if (!btn) return;
    btn->setStyleSheet("border: 2px solid #FF4444;");
}

/* ────────────────────────────────────────────────
   BOTONES BORDES VERDES
   ──────────────────────────────────────────────── */
void Header::setButtonGreen(QPushButton *btn)
{
    if (!btn) return;
    btn->setStyleSheet("border: 2px solid #16c416;");
}

/* ────────────────────────────────────────────────
   METODO QUE DECIDE QUE BOTÓN TOMA VERDE O ROJO
   ──────────────────────────────────────────────── */
   void Header::updatePanelButtonStyles(int index)
{
    // Primero todos verdes
    setButtonGreen(sceneFilterPanelBtn);
    setButtonGreen(sourceIconBtn);
    setButtonGreen(sourcePropertiesPanelBtn);
    setButtonGreen(sourceTransformPanelBtn);
    setButtonGreen(sourceFiltersPanelBtn);

    // Ahora el activo → rojo
    switch (index) {
        case 0: setButtonRed(sourceIconBtn); break;
        case 1: setButtonRed(sourcePropertiesPanelBtn); break;
        case 2: setButtonRed(sourceFiltersPanelBtn); break;
        case 3: setButtonRed(sceneFilterPanelBtn); break;
        case 4: setButtonRed(sourceTransformPanelBtn); break;
        case 5: /* misc */ break;
    }
}