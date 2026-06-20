#include "transitions.hpp"
#include "src/widgets/box.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMenu>
#include <QAction>
#include <QFrame>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QAbstractItemView>
#include <QSpinBox>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QFont>

#include <obs-frontend-api.h>
#include <obs-module.h>

#include "editor-model.hpp"


static const int TRANSITION_LABEL_WIDTH = 60;

/* ────────────────────────────────────────────────
   CONSTRUCTOR — MONTAJE DE LA CAJA TRANSITIONS
   ──────────────────────────────────────────────── */
Transitions::Transitions(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    auto *box = new Box(this);

    buildShowTransition(box);
    buildHideTransition(box);
    connectShowTransition();
    connectHideTransition();
    connectConfigButton();
    connectTimeControl();       // SHOW
    connectTimeControlHide();   // HIDE

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(box);

    /* IMPORTANTE: actualizar combo al arrancar */
    updateShowLabel();
    updateHideLabel();

    /* 🔥 NUEVO: sincronizar SIEMPRE que cambie el modelo */
    if (model) {
        QObject::connect(model, &EditorModel::modelUpdated,
                         this, [this]() {
            updateShowLabel();
            updateHideLabel();
        });
    }
}


/* ────────────────────────────────────────────────
   BLOQUE — LÍNEA SHOW TRANSITION (UI)
   ──────────────────────────────────────────────── */
void Transitions::buildShowTransition(Box *box)
{
    auto *v = qobject_cast<QVBoxLayout *>(box->layout());

    auto *lineShow   = new QWidget(box);
    auto *hLineShow  = new QHBoxLayout(lineShow);
    hLineShow->setContentsMargins(0, 0, 0, 0);
    hLineShow->setSpacing(10);
    hLineShow->setAlignment(Qt::AlignCenter);

    auto *showLabel = new QLabel(QString("<b>%1</b>").arg(obs_module_text("ShowTransition")), lineShow);
    showLabel->setFixedWidth(TRANSITION_LABEL_WIDTH);
    showLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);    
    
/* Selector (solo QComboBox estilo OBS) */
    auto *selectorShow = new QWidget(lineShow);
    auto *hShow = new QHBoxLayout(selectorShow);
    hShow->setContentsMargins(0, 0, 0, 0);
    hShow->setSpacing(4);

    /* QComboBox OBS-like (NO editable) */
    labelShow = new QComboBox(selectorShow);
    labelShow->setEditable(false);
    labelShow->setInsertPolicy(QComboBox::NoInsert);
    labelShow->setMinimumWidth(120);
    labelShow->setFixedHeight(26);
    labelShow->setFixedWidth(140);
    labelShow->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    labelShow->setStyleSheet("");
    hShow->addWidget(labelShow);

    /* BOTÓN CONFIGURACIÓN (ENGRANAJE) */
    btnConfig = new QPushButton(lineShow);
    btnConfig->setFixedSize(26, 26);
    btnConfig->setObjectName("sourcePropertiesButton");    
    btnConfig->setIconSize(QSize(18, 18));

    /* CONTROL DE TIEMPO — QSpinBox ESTILO OBS */
    timeSpin = new QSpinBox(lineShow);
    timeSpin->setRange(0, 10000);
    timeSpin->setSingleStep(50);
    timeSpin->setMinimumWidth(75);
    timeSpin->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    timeSpin->setFixedHeight(26);
    timeSpin->setSuffix(" ms");
    timeSpin->setFixedWidth(82);
    /* Padding interno para igualar a la QComboBox */
    timeSpin->setStyleSheet(
        "QSpinBox {"
        "  padding-left: 4px;"   // ajusta 4–8px según lo veas
        "  padding-right: 4px;"   // ajusta 4–8px según lo veas
        "}"
    );

    /* BOTÓN COPIAR */        
    btnCopy = new QPushButton(lineShow);
    btnCopy->setFixedSize(26, 26);
    btnCopy->setObjectName("copyButton");
    btnCopy->setIconSize(QSize(18, 18));

    /* BOTÓN PEGAR */
    btnPaste = new QPushButton(lineShow);
    btnPaste->setFixedSize(26, 26);
    btnPaste->setObjectName("pasteButton");
    btnPaste->setIconSize(QSize(18, 18));

    /* Añadir widgets a la línea */
    hLineShow->addWidget(showLabel);
    hLineShow->addWidget(selectorShow);
    hLineShow->addWidget(btnConfig);
    hLineShow->addWidget(timeSpin);
    hLineShow->addWidget(btnCopy);
    hLineShow->addWidget(btnPaste);

    if (v)
        v->addWidget(lineShow);

    /* ────────────────────────────────────────────────
    COPIAR — SHOW (clipboard universal)
    ──────────────────────────────────────────────── */
    QObject::connect(btnCopy, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *data = obs_data_create();

        // Tipo de transición (SHOW = true)
        obs_source_t *tr = obs_sceneitem_get_transition(item, true);
        if (tr) {
            const char *id = obs_source_get_unversioned_id(tr);
            obs_data_set_string(data, "transition_id", id);

            obs_data_t *settings = obs_source_get_settings(tr);
            obs_data_set_obj(data, "transition_settings", settings);
            obs_data_release(settings);
        } else {
            obs_data_set_string(data, "transition_id", "");
        }

        // Duración
        uint32_t dur = obs_sceneitem_get_transition_duration(item, true);
        obs_data_set_int(data, "transition_duration", dur);

        // Guardar en clipboard universal
        if (model->clipboardTransition)
            obs_data_release(model->clipboardTransition);

        model->clipboardTransition = data;
    });

    /* ────────────────────────────────────────────────
    PEGAR — SHOW (usa clipboard universal)
    ──────────────────────────────────────────────── */
    QObject::connect(btnPaste, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *data = model->clipboardTransition;
        if (!data)
            return;

        QString id = obs_data_get_string(data, "transition_id");
        obs_source_t *transition = nullptr;

        if (!id.isEmpty()) {
            obs_data_t *settings = obs_data_get_obj(data, "transition_settings");
            transition = obs_source_create_private(id.toUtf8().constData(),
                                                id.toUtf8().constData(),
                                                settings);
            obs_data_release(settings);
        }

        // Aplicar SIEMPRE como SHOW (true)
        obs_sceneitem_set_transition(item, true, transition);

        if (transition)
            obs_source_release(transition);

        uint32_t dur = (uint32_t)obs_data_get_int(data, "transition_duration");
        obs_sceneitem_set_transition_duration(item, true, dur);

        updateShowLabel();
        updateHideLabel();
    });

}

/* ────────────────────────────────────────────────
   BLOQUE — LÍNEA HIDE TRANSITION (UI)
   ──────────────────────────────────────────────── */
void Transitions::buildHideTransition(Box *box)
{
    auto *v = qobject_cast<QVBoxLayout *>(box->layout());

    auto *lineHide   = new QWidget(box);
    auto *hLineHide  = new QHBoxLayout(lineHide);
    hLineHide->setContentsMargins(0, 0, 0, 0);
    hLineHide->setSpacing(10);
    hLineHide->setAlignment(Qt::AlignCenter);

    auto *hideLabelTitle = new QLabel(QString("<b>%1</b>").arg(obs_module_text("HideTransition")), lineHide);
    hideLabelTitle->setFixedWidth(TRANSITION_LABEL_WIDTH);
    hideLabelTitle->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    /* Selector (QComboBox estilo OBS) */
    auto *selectorHide = new QWidget(lineHide);
    auto *hHide = new QHBoxLayout(selectorHide);
    hHide->setContentsMargins(0, 0, 0, 0);
    hHide->setSpacing(4);

    /* QComboBox OBS-like */
    labelHide = new QComboBox(selectorHide);
    labelHide->setEditable(false);
    labelHide->setInsertPolicy(QComboBox::NoInsert);
    labelHide->setMinimumWidth(120);
    labelHide->setFixedHeight(26);
    labelHide->setFixedWidth(140);
    labelHide->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    labelHide->setStyleSheet("");
    hHide->addWidget(labelHide);

    /* BOTÓN CONFIGURACIÓN (ENGRANAJE) */
    btnConfigHide = new QPushButton(lineHide);
    btnConfigHide->setFixedSize(26, 26);
    btnConfigHide->setObjectName("sourcePropertiesButton");
    btnConfigHide->setIconSize(QSize(18, 18));
    
    /* CONTROL DE TIEMPO — QSpinBox estilo OBS */
    timeSpinHide = new QSpinBox(lineHide);
    timeSpinHide->setRange(0, 10000);
    timeSpinHide->setSingleStep(50);
    timeSpinHide->setMinimumWidth(75);
    timeSpinHide->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    timeSpinHide->setFixedHeight(26);
    timeSpinHide->setSuffix(" ms");
    timeSpinHide->setFixedWidth(82);
    timeSpinHide->setStyleSheet(
        "QSpinBox {"
        "  padding-left: 4px;"
        "  padding-right: 4px;"
        "}"
    );

    /* BOTÓN COPIAR */
    btnCopyHide = new QPushButton(lineHide);
    btnCopyHide->setFixedSize(26, 26);
    btnCopyHide->setObjectName("copyButton");
    btnCopyHide->setIconSize(QSize(18, 18));


    /* BOTÓN PEGAR */
    btnPasteHide = new QPushButton(lineHide);
    btnPasteHide->setFixedSize(26, 26);
    btnPasteHide->setObjectName("pasteButton");
    btnPasteHide->setIconSize(QSize(18, 18));


    /* Añadir widgets a la línea */
    hLineHide->addWidget(hideLabelTitle);
    hLineHide->addWidget(selectorHide);
    hLineHide->addWidget(btnConfigHide);
    hLineHide->addWidget(timeSpinHide);
    hLineHide->addWidget(btnCopyHide);
    hLineHide->addWidget(btnPasteHide);

    if (v)
        v->addWidget(lineHide);

    /* ────────────────────────────────────────────────
    COPIAR — HIDE (clipboard universal)
    ──────────────────────────────────────────────── */
    QObject::connect(btnCopyHide, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *data = obs_data_create();

        // Tipo de transición (HIDE = false)
        obs_source_t *tr = obs_sceneitem_get_transition(item, false);
        if (tr) {
            const char *id = obs_source_get_unversioned_id(tr);
            obs_data_set_string(data, "transition_id", id);

            obs_data_t *settings = obs_source_get_settings(tr);
            obs_data_set_obj(data, "transition_settings", settings);
            obs_data_release(settings);
        } else {
            obs_data_set_string(data, "transition_id", "");
        }

        // Duración
        uint32_t dur = obs_sceneitem_get_transition_duration(item, false);
        obs_data_set_int(data, "transition_duration", dur);

        // Guardar en clipboard universal
        if (model->clipboardTransition)
            obs_data_release(model->clipboardTransition);

        model->clipboardTransition = data;
    });

    /* ────────────────────────────────────────────────
    PEGAR — HIDE (usa clipboard universal)
    ──────────────────────────────────────────────── */
    QObject::connect(btnPasteHide, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *data = model->clipboardTransition;
        if (!data)
            return;

        QString id = obs_data_get_string(data, "transition_id");
        obs_source_t *transition = nullptr;

        if (!id.isEmpty()) {
            obs_data_t *settings = obs_data_get_obj(data, "transition_settings");
            transition = obs_source_create_private(id.toUtf8().constData(),
                                                id.toUtf8().constData(),
                                                settings);
            obs_data_release(settings);
        }

        // Aplicar SIEMPRE como HIDE (false)
        obs_sceneitem_set_transition(item, false, transition);

        if (transition)
            obs_source_release(transition);

        uint32_t dur = (uint32_t)obs_data_get_int(data, "transition_duration");
        obs_sceneitem_set_transition_duration(item, false, dur);

        updateShowLabel();
        updateHideLabel();
    });

            
}

/* ────────────────────────────────────────────────
   BLOQUE — SELECCIÓN NATIVA DEL QCOMBOBOX
   ──────────────────────────────────────────────── */
void Transitions::connectShowTransition()
{
    QObject::connect(labelShow, qOverload<int>(&QComboBox::currentIndexChanged),
                     this,
                     [this](int index) {

        /* PROTECCIÓN CRÍTICA — EVITA CRASH AL SELECCIONAR FUENTE */
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;   // ← ESTE RETURN EVITA EL CRASH

        QString id = labelShow->itemData(index).toString();

        /* Si elige "None" */
        if (id.isEmpty()) {
            obs_sceneitem_set_transition(item, true, nullptr);
            emit model->modelUpdated();

            if (timeSpin)
                timeSpin->setValue(300);

            return;
        }

        /* Crear instancia temporal */
        obs_data_t *settings = obs_data_create();
        obs_source_t *transition =
            obs_source_create_private(id.toUtf8().constData(),
                                      id.toUtf8().constData(),
                                      settings);
        obs_data_release(settings);

        if (!transition)
            return;

        /* Aplicar transición */
        obs_sceneitem_set_transition(item, true, transition);
        obs_source_release(transition);

        emit model->modelUpdated();

        /* Actualizar duración */
        if (timeSpin) {
            int duration = obs_sceneitem_get_transition_duration(item, true);
            timeSpin->setValue(duration);
        }
    });
}

/* ────────────────────────────────────────────────
   BLOQUE — SELECCIÓN NATIVA DEL QCOMBOBOX (HIDE)
   ──────────────────────────────────────────────── */
void Transitions::connectHideTransition()
{
    QObject::connect(labelHide, qOverload<int>(&QComboBox::currentIndexChanged),
                     this,
                     [this](int index) {

        /* PROTECCIÓN CRÍTICA — EVITA CRASH AL SELECCIONAR FUENTE */
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        QString id = labelHide->itemData(index).toString();

        /* Si elige "None" */
        if (id.isEmpty()) {
            obs_sceneitem_set_transition(item, false, nullptr);
            emit model->modelUpdated();

            if (timeSpinHide)
                timeSpinHide->setValue(300);

            return;
        }

        /* Crear instancia temporal */
        obs_data_t *settings = obs_data_create();
        obs_source_t *transition =
            obs_source_create_private(id.toUtf8().constData(),
                                      id.toUtf8().constData(),
                                      settings);
        obs_data_release(settings);

        if (!transition)
            return;

        /* Aplicar transición */
        obs_sceneitem_set_transition(item, false, transition);
        obs_source_release(transition);

        emit model->modelUpdated();

        /* Actualizar duración */
        if (timeSpinHide) {
            int duration = obs_sceneitem_get_transition_duration(item, false);
            timeSpinHide->setValue(duration);
        }
    });
}

/* ────────────────────────────────────────────────
   BOTÓN ⚙ — ABRIR PROPIEDADES DE LA TRANSICIÓN
   ──────────────────────────────────────────────── */
void Transitions::connectConfigButton()
{
    // CONFIG SHOW
    QObject::connect(btnConfig, &QPushButton::clicked, this, [this]() {

        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_source_t *transition = obs_sceneitem_get_transition(item, true);
        if (!transition)
            return;

        obs_frontend_open_source_properties(transition);
    });
    // CONFIG HIDE
QObject::connect(btnConfigHide, &QPushButton::clicked, this, [this]() {

    if (!model)
        return;

    obs_sceneitem_t *item = model->selectedItem();
    if (!item)
        return;

    obs_source_t *transition = obs_sceneitem_get_transition(item, false);
    if (!transition)
        return;

    obs_frontend_open_source_properties(transition);
});

}

/* ────────────────────────────────────────────────
   CONTROL DE TIEMPO — SINCRONIZAR CON OBS (SHOW)
   ──────────────────────────────────────────────── */
void Transitions::connectTimeControl()
{
    if (!timeSpin)
        return;

    QObject::connect(timeSpin, qOverload<int>(&QSpinBox::valueChanged),
                     this,
                     [this](int value) {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        // ESCRIBIR LA DURACIÓN EN OBS
        obs_sceneitem_set_transition_duration(item, true, (uint32_t)value);
    });
}

/* ────────────────────────────────────────────────
   CONTROL DE TIEMPO — SINCRONIZAR CON OBS (HIDE)
   ──────────────────────────────────────────────── */
void Transitions::connectTimeControlHide()
{
    if (!timeSpinHide)
        return;

    QObject::connect(timeSpinHide, qOverload<int>(&QSpinBox::valueChanged),
                     this,
                     [this](int value) {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        // ESCRIBIR LA DURACIÓN EN OBS (HIDE)
        obs_sceneitem_set_transition_duration(item, false, (uint32_t)value);
    });
}

/* ────────────────────────────────────────────────
   MÉTODO — ACTUALIZAR LABEL DESDE FUERA (SHOW)
   ──────────────────────────────────────────────── */
void Transitions::updateShowLabel(const QString &)
{
    if (!model || !labelShow)
        return;

    /* No actualizar mientras el usuario está usando el combo */
    if (labelShow->view()->isVisible())
        return;    

    /* PROTECCIÓN: no emitir señales mientras actualizamos */
    QSignalBlocker blocker(labelShow);

    /* ────────────────────────────────────────────────
       REPoblar QComboBox con todas las transiciones
       (seguro gracias a QSignalBlocker)
       ──────────────────────────────────────────────── */
    labelShow->clear();
    labelShow->addItem(obs_module_text("None"), "");


    size_t idx = 0;
    const char *tid = nullptr;

    while (obs_enum_transition_types(idx++, &tid)) {
        const char *tname = obs_source_get_display_name(tid);
        labelShow->addItem(QString::fromUtf8(tname),
                           QString::fromUtf8(tid));
    }

    /* ────────────────────────────────────────────────
       OBTENER ITEM SELECCIONADO
       ──────────────────────────────────────────────── */
    obs_sceneitem_t *item = model->selectedItem();
    if (!item) {
        labelShow->setCurrentIndex(0);
        if (timeSpin)
            timeSpin->setValue(300);
        return;
    }

    obs_source_t *current = obs_sceneitem_get_transition(item, true);
    if (!current) {
        labelShow->setCurrentIndex(0);

        if (timeSpin) {
            uint32_t duration = obs_sceneitem_get_transition_duration(item, true);
            if (duration == 0)
                duration = obs_frontend_get_transition_duration();
            timeSpin->setValue((int)duration);
        }
        return;
    }

    /* ────────────────────────────────────────────────
       SELECCIONAR TRANSICIÓN ACTUAL
       ──────────────────────────────────────────────── */
    const char *id = obs_source_get_unversioned_id(current);

    int index = labelShow->findData(QString::fromUtf8(id));
    if (index >= 0)
        labelShow->setCurrentIndex(index);

    /* ────────────────────────────────────────────────
       ACTUALIZAR DURACIÓN
       ──────────────────────────────────────────────── */
    if (timeSpin) {
        uint32_t duration = obs_sceneitem_get_transition_duration(item, true);
        if (duration == 0)
            duration = obs_frontend_get_transition_duration();
        timeSpin->setValue((int)duration);
    }
}
/* ────────────────────────────────────────────────
   MÉTODO — ACTUALIZAR LABEL DESDE FUERA (HIDE)
   ──────────────────────────────────────────────── */
void Transitions::updateHideLabel(const QString &)
{
    if (!model || !labelHide)
        return;

    /* No actualizar mientras el usuario está usando el combo */
    if (labelHide->view()->isVisible())
        return;

    /* PROTECCIÓN: no emitir señales mientras actualizamos */
    QSignalBlocker blocker(labelHide);

    /* ────────────────────────────────────────────────
       REPoblar QComboBox con todas las transiciones
       ──────────────────────────────────────────────── */
    labelHide->clear();
    labelHide->addItem(obs_module_text("None"), "");


    size_t idx = 0;
    const char *tid = nullptr;

    while (obs_enum_transition_types(idx++, &tid)) {
        const char *tname = obs_source_get_display_name(tid);
        labelHide->addItem(QString::fromUtf8(tname),
                           QString::fromUtf8(tid));
    }

    /* ────────────────────────────────────────────────
       OBTENER ITEM SELECCIONADO
       ──────────────────────────────────────────────── */
    obs_sceneitem_t *item = model->selectedItem();
    if (!item) {
        labelHide->setCurrentIndex(0);
        if (timeSpinHide)
            timeSpinHide->setValue(300);
        return;
    }

    obs_source_t *current = obs_sceneitem_get_transition(item, false);
    if (!current) {
        labelHide->setCurrentIndex(0);

        if (timeSpinHide) {
            uint32_t duration = obs_sceneitem_get_transition_duration(item, false);
            if (duration == 0)
                duration = obs_frontend_get_transition_duration();
            timeSpinHide->setValue((int)duration);
        }
        return;
    }

    /* ────────────────────────────────────────────────
       SELECCIONAR TRANSICIÓN ACTUAL
       ──────────────────────────────────────────────── */
    const char *id = obs_source_get_unversioned_id(current);

    int index = labelHide->findData(QString::fromUtf8(id));
    if (index >= 0)
        labelHide->setCurrentIndex(index);

    /* ────────────────────────────────────────────────
       ACTUALIZAR DURACIÓN
       ──────────────────────────────────────────────── */
    if (timeSpinHide) {
        uint32_t duration = obs_sceneitem_get_transition_duration(item, false);
        if (duration == 0)
            duration = obs_frontend_get_transition_duration();
        timeSpinHide->setValue((int)duration);
    }
}


