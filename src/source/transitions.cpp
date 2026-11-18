#include "transitions.hpp"
#include "src/widgets/box.hpp"
#include "editor-ui-helpers.hpp"

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

#include <cstdio>

#include <obs-frontend-api.h>
#include <obs-module.h>

#include "editor-model.hpp"

static obs_data_t *captureTransitions(obs_sceneitem_t *item);
static void pushTransitionsUndo(const char *nameKey, obs_sceneitem_t *item,
                                obs_data_t *before, obs_data_t *after);

static const int TRANSITION_LABEL_WIDTH = 60;

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
    connectTimeControl();
    connectTimeControlHide();

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(box);

    updateShowLabel();
    updateHideLabel();

    if (model) {
        QObject::connect(model, &EditorModel::modelUpdated,
                         this, [this]() {
            updateShowLabel();
            updateHideLabel();
            refreshControlStates();
        });
    }

    connect(btnCopy, &QPushButton::clicked,
            this, &Transitions::refreshControlStates);
    connect(btnPaste, &QPushButton::clicked,
            this, &Transitions::refreshControlStates);
    connect(btnCopyHide, &QPushButton::clicked,
            this, &Transitions::refreshControlStates);
    connect(btnPasteHide, &QPushButton::clicked,
            this, &Transitions::refreshControlStates);

    refreshControlStates();
}


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
    
    auto *selectorShow = new QWidget(lineShow);
    auto *hShow = new QHBoxLayout(selectorShow);
    hShow->setContentsMargins(0, 0, 0, 0);
    hShow->setSpacing(4);

    labelShow = new QComboBox(selectorShow);
    labelShow->setEditable(false);
    labelShow->setInsertPolicy(QComboBox::NoInsert);
    labelShow->setMinimumWidth(120);
    labelShow->setFixedHeight(26);
    labelShow->setFixedWidth(140);
    labelShow->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    labelShow->setStyleSheet("");
    hShow->addWidget(labelShow);

    btnConfig = new QPushButton(lineShow);
    btnConfig->setFixedSize(26, 26);
    btnConfig->setObjectName("sourcePropertiesButton");    
    btnConfig->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnConfig,
        editorui::GetIconPath("Properties.svg").c_str()
    );

    timeSpin = new QSpinBox(lineShow);
    timeSpin->setRange(0, 10000);
    timeSpin->setSingleStep(50);
    timeSpin->setMinimumWidth(75);
    timeSpin->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    timeSpin->setFixedHeight(26);
    timeSpin->setSuffix(" ms");
    timeSpin->setFixedWidth(82);
    timeSpin->setStyleSheet(
        "QSpinBox {"
        "  padding-left: 4px;"
        "  padding-right: 4px;"
        "}"
    );

    btnCopy = new QPushButton(lineShow);
    btnCopy->setFixedSize(26, 26);
    btnCopy->setObjectName("copyButton");
    btnCopy->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnCopy,
        editorui::GetIconPath("Copy.svg").c_str()
    );

    btnPaste = new QPushButton(lineShow);
    btnPaste->setFixedSize(26, 26);
    btnPaste->setObjectName("pasteButton");
    btnPaste->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnPaste,
        editorui::GetIconPath("Paste.svg").c_str()
    );

    hLineShow->addWidget(showLabel);
    hLineShow->addWidget(selectorShow);
    hLineShow->addWidget(btnConfig);
    hLineShow->addWidget(timeSpin);
    hLineShow->addWidget(btnCopy);
    hLineShow->addWidget(btnPaste);

    if (v)
        v->addWidget(lineShow);

    QObject::connect(btnCopy, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *data = obs_data_create();

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

        uint32_t dur = obs_sceneitem_get_transition_duration(item, true);
        obs_data_set_int(data, "transition_duration", dur);

        if (model->clipboardTransition)
            obs_data_release(model->clipboardTransition);

        model->clipboardTransition = data;
    });

    QObject::connect(btnPaste, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *data = model->clipboardTransition;
        if (!data)
            return;

        obs_data_t *before = captureTransitions(item);

        QString id = obs_data_get_string(data, "transition_id");
        obs_source_t *transition = nullptr;

        if (!id.isEmpty()) {
            obs_data_t *settings = obs_data_get_obj(data, "transition_settings");
            transition = obs_source_create_private(id.toUtf8().constData(),
                                                id.toUtf8().constData(),
                                                settings);
            obs_data_release(settings);
        }

        obs_sceneitem_set_transition(item, true, transition);

        if (transition)
            obs_source_release(transition);

        uint32_t dur = (uint32_t)obs_data_get_int(data, "transition_duration");
        obs_sceneitem_set_transition_duration(item, true, dur);

        updateShowLabel();
        updateHideLabel();

        obs_data_t *after = captureTransitions(item);
        pushTransitionsUndo("UndoShowTransition", item, before, after);
        obs_data_release(before);
        obs_data_release(after);
    });

}

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

    auto *selectorHide = new QWidget(lineHide);
    auto *hHide = new QHBoxLayout(selectorHide);
    hHide->setContentsMargins(0, 0, 0, 0);
    hHide->setSpacing(4);

    labelHide = new QComboBox(selectorHide);
    labelHide->setEditable(false);
    labelHide->setInsertPolicy(QComboBox::NoInsert);
    labelHide->setMinimumWidth(120);
    labelHide->setFixedHeight(26);
    labelHide->setFixedWidth(140);
    labelHide->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    labelHide->setStyleSheet("");
    hHide->addWidget(labelHide);

    btnConfigHide = new QPushButton(lineHide);
    btnConfigHide->setFixedSize(26, 26);
    btnConfigHide->setObjectName("sourcePropertiesButton");
    btnConfigHide->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnConfigHide,
        editorui::GetIconPath("Properties.svg").c_str()
    );
    
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

    btnCopyHide = new QPushButton(lineHide);
    btnCopyHide->setFixedSize(26, 26);
    btnCopyHide->setObjectName("copyButton");
    btnCopyHide->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnCopyHide,
        editorui::GetIconPath("Copy.svg").c_str()
    );

    btnPasteHide = new QPushButton(lineHide);
    btnPasteHide->setFixedSize(26, 26);
    btnPasteHide->setObjectName("pasteButton");
    btnPasteHide->setIconSize(QSize(18, 18));
    editorui::SetButtonIcon(
        btnPasteHide,
        editorui::GetIconPath("Paste.svg").c_str()
    );

    hLineHide->addWidget(hideLabelTitle);
    hLineHide->addWidget(selectorHide);
    hLineHide->addWidget(btnConfigHide);
    hLineHide->addWidget(timeSpinHide);
    hLineHide->addWidget(btnCopyHide);
    hLineHide->addWidget(btnPasteHide);

    if (v)
        v->addWidget(lineHide);

    QObject::connect(btnCopyHide, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *data = obs_data_create();

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

        uint32_t dur = obs_sceneitem_get_transition_duration(item, false);
        obs_data_set_int(data, "transition_duration", dur);

        if (model->clipboardTransition)
            obs_data_release(model->clipboardTransition);

        model->clipboardTransition = data;
    });

    QObject::connect(btnPasteHide, &QPushButton::clicked, this, [this]() {
        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *data = model->clipboardTransition;
        if (!data)
            return;

        obs_data_t *before = captureTransitions(item);

        QString id = obs_data_get_string(data, "transition_id");
        obs_source_t *transition = nullptr;

        if (!id.isEmpty()) {
            obs_data_t *settings = obs_data_get_obj(data, "transition_settings");
            transition = obs_source_create_private(id.toUtf8().constData(),
                                                id.toUtf8().constData(),
                                                settings);
            obs_data_release(settings);
        }

        obs_sceneitem_set_transition(item, false, transition);

        if (transition)
            obs_source_release(transition);

        uint32_t dur = (uint32_t)obs_data_get_int(data, "transition_duration");
        obs_sceneitem_set_transition_duration(item, false, dur);

        updateShowLabel();
        updateHideLabel();

        obs_data_t *after = captureTransitions(item);
        pushTransitionsUndo("UndoHideTransition", item, before, after);
        obs_data_release(before);
        obs_data_release(after);
    });

            
}

static void storeTransition(obs_data_t *d, const char *prefix,
                            obs_sceneitem_t *item, bool show)
{
    char key[64];
    obs_source_t *tr = obs_sceneitem_get_transition(item, show);

    if (!tr) {
        snprintf(key, sizeof(key), "%s_id", prefix);
        obs_data_set_string(d, key, "");
        return;
    }

    snprintf(key, sizeof(key), "%s_id", prefix);
    obs_data_set_string(d, key, obs_source_get_id(tr));

    obs_data_t *settings = obs_source_get_settings(tr);
    if (settings) {
        snprintf(key, sizeof(key), "%s_settings", prefix);
        obs_data_set_obj(d, key, settings);
        obs_data_release(settings);
    }

    snprintf(key, sizeof(key), "%s_dur", prefix);
    obs_data_set_int(d, key, obs_sceneitem_get_transition_duration(item, show));
}

static obs_data_t *captureTransitions(obs_sceneitem_t *item)
{
    obs_data_t *d = obs_data_create();

    obs_scene_t *sc = obs_sceneitem_get_scene(item);
    obs_source_t *scs = sc ? obs_scene_get_source(sc) : nullptr;
    obs_data_set_string(d, "scene_uuid", scs ? obs_source_get_uuid(scs) : "");
    obs_data_set_int(d, "item_id", obs_sceneitem_get_id(item));

    storeTransition(d, "show", item, true);
    storeTransition(d, "hide", item, false);
    return d;
}

static void applyTransition(obs_data_t *d, const char *prefix,
                            obs_sceneitem_t *item, bool show)
{
    char key[64];

    snprintf(key, sizeof(key), "%s_id", prefix);
    const char *id = obs_data_get_string(d, key);

    if (!id || !*id) {
        obs_sceneitem_set_transition(item, show, nullptr);
        return;
    }

    obs_data_t *settings;
    snprintf(key, sizeof(key), "%s_settings", prefix);
    settings = obs_data_get_obj(d, key);
    if (!settings)
        settings = obs_data_create();

    obs_source_t *transition = obs_source_create_private(id, id, settings);
    obs_data_release(settings);

    if (!transition)
        return;

    obs_sceneitem_set_transition(item, show, transition);

    snprintf(key, sizeof(key), "%s_dur", prefix);
    obs_sceneitem_set_transition_duration(item, show,
                                          (uint32_t)obs_data_get_int(d, key));

    obs_source_release(transition);
}

static void transitions_undo_cb(const char *data)
{
    obs_data_t *d = obs_data_create_from_json(data);
    if (!d)
        return;

    const char *uuid = obs_data_get_string(d, "scene_uuid");
    int64_t itemId = obs_data_get_int(d, "item_id");

    obs_source_t *sceneSrc =
        (uuid && *uuid) ? obs_get_source_by_uuid(uuid) : nullptr;
    obs_scene_t *sc = sceneSrc ? obs_scene_from_source(sceneSrc) : nullptr;
    obs_sceneitem_t *item = sc ? obs_scene_find_sceneitem_by_id(sc, itemId) : nullptr;

    if (item) {
        applyTransition(d, "show", item, true);
        applyTransition(d, "hide", item, false);
    }

    if (sceneSrc)
        obs_source_release(sceneSrc);
    obs_data_release(d);

    if (EditorModel *m = EditorModel::instanceForUndo()) {
        m->refreshItemCaches();
        m->modelUpdated();
    }
}

static void pushTransitionsUndo(const char *nameKey, obs_sceneitem_t *item,
                                obs_data_t *before, obs_data_t *after)
{
    const char *b = obs_data_get_json(before);
    const char *a = obs_data_get_json(after);

    if (!b || !a || strcmp(b, a) == 0)
        return;

    obs_frontend_add_undo_redo_action(obs_module_text(nameKey),
                                      transitions_undo_cb, transitions_undo_cb,
                                      b, a, false);
}

void Transitions::connectShowTransition()
{
    QObject::connect(labelShow, qOverload<int>(&QComboBox::currentIndexChanged),
                     this,
                     [this](int index) {

        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *before = captureTransitions(item);

        QString id = labelShow->itemData(index).toString();

        if (id.isEmpty()) {
            obs_sceneitem_set_transition(item, true, nullptr);

            m_applyingTransition = true;
            if (timeSpin)
                timeSpin->setValue(300);
            m_applyingTransition = false;

            obs_data_t *after = captureTransitions(item);
            pushTransitionsUndo("UndoShowTransition", item, before, after);
            obs_data_release(before);
            obs_data_release(after);

            emit model->modelUpdated();
            return;
        }

        obs_data_t *settings = obs_data_create();
        obs_source_t *transition =
            obs_source_create_private(id.toUtf8().constData(),
                                      id.toUtf8().constData(),
                                      settings);
        obs_data_release(settings);

        if (!transition)
            return;

        obs_sceneitem_set_transition(item, true, transition);
        obs_source_release(transition);

        obs_data_t *after = captureTransitions(item);
        pushTransitionsUndo("UndoShowTransition", item, before, after);
        obs_data_release(before);
        obs_data_release(after);

        emit model->modelUpdated();

        if (timeSpin) {
            int duration = obs_sceneitem_get_transition_duration(item, true);
            timeSpin->setValue(duration);
        }
    });
}

void Transitions::connectHideTransition()
{
    QObject::connect(labelHide, qOverload<int>(&QComboBox::currentIndexChanged),
                     this,
                     [this](int index) {

        if (!model)
            return;

        obs_sceneitem_t *item = model->selectedItem();
        if (!item)
            return;

        obs_data_t *before = captureTransitions(item);

        QString id = labelHide->itemData(index).toString();

        if (id.isEmpty()) {
            obs_sceneitem_set_transition(item, false, nullptr);

            m_applyingTransition = true;
            if (timeSpinHide)
                timeSpinHide->setValue(300);
            m_applyingTransition = false;

            obs_data_t *after = captureTransitions(item);
            pushTransitionsUndo("UndoHideTransition", item, before, after);
            obs_data_release(before);
            obs_data_release(after);

            emit model->modelUpdated();
            return;
        }

        obs_data_t *settings = obs_data_create();
        obs_source_t *transition =
            obs_source_create_private(id.toUtf8().constData(),
                                      id.toUtf8().constData(),
                                      settings);
        obs_data_release(settings);

        if (!transition)
            return;

        obs_sceneitem_set_transition(item, false, transition);
        obs_source_release(transition);

        obs_data_t *after = captureTransitions(item);
        pushTransitionsUndo("UndoHideTransition", item, before, after);
        obs_data_release(before);
        obs_data_release(after);

        emit model->modelUpdated();

        if (timeSpinHide) {
            int duration = obs_sceneitem_get_transition_duration(item, false);
            timeSpinHide->setValue(duration);
        }
    });
}

void Transitions::connectConfigButton()
{
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

        uint32_t old = obs_sceneitem_get_transition_duration(item, true);
        if (old == (uint32_t)value)
            return;

        obs_sceneitem_set_transition_duration(item, true, (uint32_t)value);

        if (m_applyingTransition)
            return;

        model->pushItemPropUndo("UndoShowTransition", item,
                                "transition_duration_show", old, value, true);
    });
}

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

        uint32_t old = obs_sceneitem_get_transition_duration(item, false);
        if (old == (uint32_t)value)
            return;

        obs_sceneitem_set_transition_duration(item, false, (uint32_t)value);

        if (m_applyingTransition)
            return;

        model->pushItemPropUndo("UndoHideTransition", item,
                                "transition_duration_hide", old, value, true);
    });
}

void Transitions::updateShowLabel(const QString &)
{
    if (!model || !labelShow)
        return;

    if (labelShow->view()->isVisible())
        return;    

    QSignalBlocker blocker(labelShow);

    labelShow->clear();
    labelShow->addItem(obs_module_text("None"), "");


    size_t idx = 0;
    const char *tid = nullptr;

    while (obs_enum_transition_types(idx++, &tid)) {
        const char *tname = obs_source_get_display_name(tid);
        labelShow->addItem(QString::fromUtf8(tname),
                           QString::fromUtf8(tid));
    }

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

    const char *id = obs_source_get_unversioned_id(current);

    int index = labelShow->findData(QString::fromUtf8(id));
    if (index >= 0)
        labelShow->setCurrentIndex(index);

    if (timeSpin) {
        uint32_t duration = obs_sceneitem_get_transition_duration(item, true);
        if (duration == 0)
            duration = obs_frontend_get_transition_duration();
        timeSpin->setValue((int)duration);
    }
}
void Transitions::updateHideLabel(const QString &)
{
    if (!model || !labelHide)
        return;

    if (labelHide->view()->isVisible())
        return;

    QSignalBlocker blocker(labelHide);

    labelHide->clear();
    labelHide->addItem(obs_module_text("None"), "");


    size_t idx = 0;
    const char *tid = nullptr;

    while (obs_enum_transition_types(idx++, &tid)) {
        const char *tname = obs_source_get_display_name(tid);
        labelHide->addItem(QString::fromUtf8(tname),
                           QString::fromUtf8(tid));
    }

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

    const char *id = obs_source_get_unversioned_id(current);

    int index = labelHide->findData(QString::fromUtf8(id));
    if (index >= 0)
        labelHide->setCurrentIndex(index);

    if (timeSpinHide) {
        uint32_t duration = obs_sceneitem_get_transition_duration(item, false);
        if (duration == 0)
            duration = obs_frontend_get_transition_duration();
        timeSpinHide->setValue((int)duration);
    }
}

void Transitions::refreshControlStates()
{
    const bool hasItem = model && model->selectedItem() != nullptr;

    const QString showId =
        labelShow ? labelShow->currentData().toString() : QString();
    const QString hideId =
        labelHide ? labelHide->currentData().toString() : QString();

    const bool showNone = showId.isEmpty();
    const bool hideNone = hideId.isEmpty();

    const bool showNoCfg =
        showNone || showId == "cut_transition" || showId == "fade_transition";
    const bool hideNoCfg =
        hideNone || hideId == "cut_transition" || hideId == "fade_transition";

    if (btnConfig)
        btnConfig->setEnabled(hasItem && !showNoCfg);
    if (timeSpin)
        timeSpin->setEnabled(
            hasItem && !showNone && showId != "cut_transition");
    if (btnCopy)
        btnCopy->setEnabled(hasItem && !showNone);

    if (btnConfigHide)
        btnConfigHide->setEnabled(hasItem && !hideNoCfg);
    if (timeSpinHide)
        timeSpinHide->setEnabled(
            hasItem && !hideNone && hideId != "cut_transition");
    if (btnCopyHide)
        btnCopyHide->setEnabled(hasItem && !hideNone);

    bool hasBuffer = false;
    if (model && model->clipboardTransition) {
        const char *bid =
            obs_data_get_string(model->clipboardTransition, "transition_id");
        hasBuffer = bid && *bid;
    }

    if (btnPaste)
        btnPaste->setEnabled(hasItem && hasBuffer);
    if (btnPasteHide)
        btnPasteHide->setEnabled(hasItem && hasBuffer);
}


