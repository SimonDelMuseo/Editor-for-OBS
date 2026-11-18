#include "header.hpp"
#include "editor-model.hpp"
#include "editor-ui-helpers.hpp"

#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QMetaObject>
#include <QEvent>
#include <QVariantAnimation>
#include <QTimer>
#include <QEasingCurve>
#include <QCoreApplication>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <cmath>

#include "preview-source.hpp"

extern "C" {
#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
}

static bool s_editingSourceName = false;

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

    auto *sceneRow = new QHBoxLayout();
    sceneRow->setSpacing(6);

    sceneIconBtn = new QPushButton(this);
    sceneIconBtn->setFlat(true);
    sceneIconBtn->setFixedSize(26, 26);
    sceneIconBtn->setFocusPolicy(Qt::NoFocus);
    sceneIconBtn->setObjectName("sceneIconBtn");

    sceneIconBtn->setStyleSheet(
        "#sceneIconBtn:hover { background: transparent; }"
    );

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

    sceneFilterPanelBtn = new QPushButton(this);
    sceneFilterPanelBtn->setFlat(true);
    sceneFilterPanelBtn->setFixedSize(26, 26);
    sceneFilterPanelBtn->setObjectName("sourceFiltersButton");
    sceneFilterPanelBtn->setIconSize(QSize(18, 18));
    sceneFilterPanelBtn->setToolTip(obs_module_text("SceneFiltersTab"));
    editorui::SetButtonIcon(
        sceneFilterPanelBtn,
        editorui::GetIconPath("Filter.svg").c_str()
    );

    sceneNotesBtn = new QPushButton(this);
    sceneNotesBtn->setFlat(true);
    sceneNotesBtn->setFixedSize(26, 26);
    sceneNotesBtn->setObjectName("notesButton");
    sceneNotesBtn->setIconSize(QSize(18, 18));
    sceneNotesBtn->setToolTip(obs_module_text("SceneNotesTab"));
    editorui::SetButtonIcon(
        sceneNotesBtn,
        editorui::GetIconPath("Notes.svg").c_str()
    );

    sourceCountEdit = new QLineEdit(this);
    sourceCountEdit->setObjectName("sourceCountEdit");
    sourceCountEdit->setReadOnly(true);
    sourceCountEdit->setFocusPolicy(Qt::NoFocus);
    sourceCountEdit->setCursor(Qt::ArrowCursor);
    sourceCountEdit->setFixedWidth(26 * 3 + 6 * 2);
    sourceCountEdit->setFixedHeight(26);
    sourceCountEdit->setAlignment(Qt::AlignCenter);



    sceneRow->addWidget(sceneIconBtn);
    sceneRow->addWidget(sceneCombo);
    sceneRow->addWidget(sourceCountEdit);
    sceneRow->addWidget(sceneFilterPanelBtn);
    sceneRow->addWidget(sceneNotesBtn);

    auto *sourceRow = new QHBoxLayout();
    sourceRow->setSpacing(6);

    sourceIconBtn = new QPushButton(this);
    sourceIconBtn->setFlat(true);
    sourceIconBtn->setFixedSize(26, 26);
    sourceIconBtn->setFocusPolicy(Qt::NoFocus);
    sourceIconBtn->setObjectName("sourceIconBtn");
    sourceIconBtn->setToolTip(obs_module_text("SourcePanelTT"));

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

    previewBtn = new QPushButton(this);
    previewBtn->setFlat(true);
    previewBtn->setFixedSize(26, 26);
    previewBtn->setFocusPolicy(Qt::NoFocus);
    previewBtn->setObjectName("previewSourceButton");
    previewBtn->setIconSize(QSize(18, 18));
    previewBtn->setToolTip(obs_module_text("PreviewTitle"));

    sourcePropertiesPanelBtn = new QPushButton(this);
    sourcePropertiesPanelBtn->setFlat(true);
    sourcePropertiesPanelBtn->setFixedSize(26, 26);
    sourcePropertiesPanelBtn->setObjectName("sourcePropertiesButton");
    sourcePropertiesPanelBtn->setIconSize(QSize(18, 18));
    sourcePropertiesPanelBtn->setToolTip(obs_module_text("PropertiesTab"));
    editorui::SetButtonIcon(
        sourcePropertiesPanelBtn,
        editorui::GetIconPath("Properties.svg").c_str()
    );

    sourceTransformPanelBtn = new QPushButton(this);
    sourceTransformPanelBtn->setFlat(true);
    sourceTransformPanelBtn->setFixedSize(26, 26);
    sourceTransformPanelBtn->setObjectName("transformButton");
    sourceTransformPanelBtn->setIconSize(QSize(18, 18));
    sourceTransformPanelBtn->setToolTip(obs_module_text("TransformTab"));
    editorui::SetButtonIcon(
        sourceTransformPanelBtn,
        editorui::GetIconPath("Transform.svg").c_str()
    );

    sourceFiltersPanelBtn = new QPushButton(this);
    sourceFiltersPanelBtn->setFlat(true);
    sourceFiltersPanelBtn->setFixedSize(26, 26);
    sourceFiltersPanelBtn->setObjectName("sourceFiltersButton");
    sourceFiltersPanelBtn->setIconSize(QSize(18, 18));
    sourceFiltersPanelBtn->setToolTip(obs_module_text("SourceFiltersTT"));
    editorui::SetButtonIcon(
        sourceFiltersPanelBtn,
        editorui::GetIconPath("Filter.svg").c_str()
    );

    sourceNotesBtn = new QPushButton(this);
    sourceNotesBtn->setFlat(true);
    sourceNotesBtn->setFixedSize(26, 26);
    sourceNotesBtn->setObjectName("notesButton");
    sourceNotesBtn->setIconSize(QSize(18, 18));
    sourceNotesBtn->setToolTip(obs_module_text("SourceNotesTab"));
    editorui::SetButtonIcon(
        sourceNotesBtn,
        editorui::GetIconPath("Notes.svg").c_str()
    );
    
    sourceRow->addWidget(sourceIconBtn);
    sourceRow->addWidget(sourceCombo);
    sourceRow->addWidget(previewBtn);
    sourceRow->addWidget(sourcePropertiesPanelBtn);
    sourceRow->addWidget(sourceTransformPanelBtn);
    sourceRow->addWidget(sourceFiltersPanelBtn);
    sourceRow->addWidget(sourceNotesBtn);

    mainLayout->addLayout(sceneRow);
    mainLayout->addLayout(sourceRow);

    connect(model, &EditorModel::sceneChanged,
            this,  &Header::refreshSceneName);

    connect(model, &EditorModel::sceneChanged,
            this,  &Header::refreshPreviewBtn);

    connect(model, &EditorModel::sceneListChanged,
            this, &Header::refreshSceneList);

    connect(model, &EditorModel::sceneChanged,
            this, &Header::refreshSourceCount);
    connect(model, &EditorModel::sceneItemsChanged,
            this, &Header::refreshSourceCount);

    connect(sceneCombo, QOverload<int>::of(&QComboBox::activated),
            this, &Header::onSceneComboActivated);

    connect(sceneCombo->lineEdit(), &QLineEdit::textEdited,
            this, [this]() { editingSceneName = true; });

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

    previewBlink = new QVariantAnimation(this);
    previewBlink->setStartValue(0.0);
    previewBlink->setEndValue(1.0);
    previewBlink->setDuration(2000);
    previewBlink->setLoopCount(-1);
    previewBlink->setEasingCurve(QEasingCurve::InOutSine);
    connect(previewBlink, &QVariantAnimation::valueChanged,
            this, [this](const QVariant &v) {
                if (previewState != 2)
                    return;
                const double t = v.toDouble();
                const double k =
                    0.5 - 0.5 * std::cos(t * 6.283185307179586);
                auto mix = [](int a, int b, double u) {
                    return int(a + (b - a) * u + 0.5);
                };
                const QColor c(mix(0xe0, 0, k), mix(0xb3, 0, k),
                               mix(0x5e, 0, k));
                previewBtn->setStyleSheet(
                    QStringLiteral("QPushButton { border: 2px solid %1; }")
                        .arg(c.name()));
            });

    previewAutoClose = new QTimer(this);
    previewAutoClose->setSingleShot(true);
    previewAutoClose->setInterval(300);
    connect(previewAutoClose, &QTimer::timeout, this, [this, model]() {
        obs_source_t *src = model ? model->selectedSource() : nullptr;
        const bool hasVideo =
            src && (obs_source_get_output_flags(src) & OBS_SOURCE_VIDEO);
        if (!hasVideo && previewWin && previewWin->isOpen())
            previewWin->closeWindow();
    });

    connect(previewBtn, &QPushButton::clicked, this, [this, model]() {
        if (previewState == 1 || !model)
            return;
        if (!previewWin) {
            previewWin = new PreviewSource(nullptr);
            connect(previewWin, &PreviewSource::windowOpened,
                    this, &Header::refreshPreviewBtn);
            connect(previewWin, &PreviewSource::windowClosed,
                    this, &Header::refreshPreviewBtn);
        }
        if (previewState == 2)
            previewWin->open(model->selectedSource());
        else
            previewWin->closeWindow();
    });

    connect(model, &EditorModel::modelUpdated,
            this, &Header::refreshPreviewBtn);

    connect(model, &EditorModel::modelUpdated,
            this, &Header::refreshPropertiesBtnState);

    obs_frontend_add_event_callback(Header::frontend_cb, this);
    sceneCombo->lineEdit()->installEventFilter(this);

    refreshSceneName();
    refreshSourceCount();
    populateSceneList();
    refreshSourceName();
    refreshPreviewBtn();
}

Header::~Header()
{
    delete previewWin;
    previewWin = nullptr;

    obs_frontend_remove_event_callback(Header::frontend_cb, this);
}

void Header::frontend_cb(enum obs_frontend_event event, void *data)
{
    auto *self = static_cast<Header *>(data);
    if (!self)
        return;

    if (event == OBS_FRONTEND_EVENT_EXIT) {
        PreviewSource::clearStoredGeometry();
        return;
    }

    if (event != OBS_FRONTEND_EVENT_FINISHED_LOADING &&
        event != OBS_FRONTEND_EVENT_THEME_CHANGED)
        return;

    QMetaObject::invokeMethod(self, [self]() {
        QWidget *main =
            static_cast<QWidget *>(obs_frontend_get_main_window());
        if (main) {
            QVariant v = main->property("sceneIcon");
            if (v.isValid()) {
                QIcon icon = v.value<QIcon>();
                if (!icon.isNull()) {
                    self->sceneIconBtn->setIcon(icon);
                    self->sceneIconBtn->setIconSize(QSize(18, 18));
                }
            }
        }

        self->refreshSourceIcon();
        self->refreshPreviewBtn();
        self->applyPreviewBtnIcon();
        editorui::RefreshButtonIcons();
        self->refreshPropertiesBtnState();
    }, Qt::QueuedConnection);
}

bool Header::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == sceneCombo->lineEdit()) {
        if (event->type() == QEvent::FocusOut)
            editingSceneName = false;
    }
    return QFrame::eventFilter(watched, event);
}

void Header::populateSceneList()
{
    if (editingSceneName || !sceneCombo)
        return;

    sceneCombo->blockSignals(true);

    QString current = model ? model->sceneName() : QString();
    sceneCombo->clear();

    obs_frontend_source_list list = {};
    obs_frontend_get_scenes(&list);
    for (size_t i = 0; i < list.sources.num; i++) {
        obs_source_t *src = list.sources.array[i];
        if (!src)
            continue;
        const char *name = obs_source_get_name(src);
        if (name)
            sceneCombo->addItem(QString::fromUtf8(name));
    }
    obs_frontend_source_list_free(&list);

    int idx = sceneCombo->findText(current);
    if (idx >= 0) {
        sceneCombo->setCurrentIndex(idx);
    } else if (!current.isEmpty()) {
        sceneCombo->setCurrentIndex(-1);
        sceneCombo->lineEdit()->setText(current);
    }

    sceneCombo->blockSignals(false);
}

void Header::refreshSceneList()
{
    populateSceneList();
}

void Header::onSceneComboActivated(int index)
{
    if (!sceneCombo || index < 0)
        return;

    editingSceneName = false;
    QString name = sceneCombo->itemText(index);
    if (name.isEmpty() || !model)
        return;

    obs_frontend_source_list list = {};
    obs_frontend_get_scenes(&list);
    for (size_t i = 0; i < list.sources.num; i++) {
        obs_source_t *src = list.sources.array[i];
        if (!src)
            continue;
        const char *n = obs_source_get_name(src);
        if (n && name == QString::fromUtf8(n)) {
            obs_frontend_set_current_scene(src);
            break;
        }
    }
    obs_frontend_source_list_free(&list);
}


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

    const char *id = obs_source_get_unversioned_id(src);
    if (!id) {
        sourceIconBtn->setIcon(QIcon());
        return;
    }

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



void Header::paintEvent(QPaintEvent *e)
{
    QFrame::paintEvent(e);
}

void Header::refreshSceneName()
{
    sceneCombo->blockSignals(true);

    sceneCombo->clear();
    sceneCombo->addItem("");
    sceneCombo->lineEdit()->setText(model->sceneName());

    sceneCombo->blockSignals(false);
}

static bool count_item_cb(obs_scene_t *, obs_sceneitem_t *item, void *param);

static void count_scene_items(obs_scene_t *scene, int *count)
{
    obs_scene_enum_items(scene, count_item_cb, count);
}

static bool count_item_cb(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
    int *count = static_cast<int *>(param);
    ++*count;
    if (obs_sceneitem_is_group(item))
        obs_sceneitem_group_enum_items(item, count_item_cb, param);
    return true;
}

void Header::refreshSourceCount()
{
    if (!sourceCountEdit)
        return;

    int count = 0;
    if (obs_scene_t *sc = model ? model->currentScene() : nullptr)
        count_scene_items(sc, &count);

    sourceCountEdit->setText(
        QStringLiteral("%1 %2")
            .arg(count)
            .arg(QString::fromUtf8(obs_module_text("SourceCounterSuffix"))));
}

void Header::refreshSourceName()
{
    if (s_editingSourceName)
        return;

    sourceCombo->blockSignals(true);

    sourceCombo->clear();
    sourceCombo->addItem("");

    const bool hasSource = model->selectedSource() != nullptr;

    if (hasSource) {
        sourceCombo->setEnabled(true);
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
        sourceCombo->lineEdit()->setText(model->sourceName());
    } else {
        sourceCombo->lineEdit()->setText(obs_module_text("NoSourceSelected"));
        sourceCombo->setEnabled(false);
        sourceCombo->setStyleSheet(
            "QComboBox { border: 2px solid #e0b35e; background: transparent; }"
            "QComboBox::drop-down { width: 0px; border: none; }"
            "QComboBox::down-arrow { image: none; }"
        );
        sourceCombo->lineEdit()->setStyleSheet(
            "QLineEdit { background: transparent; border: none;"
            " color: #e0b35e; font-weight: bold; }"
        );
    }

    sourceCombo->blockSignals(false);
}

static QIcon AtenuatedIcon(const QString &iconPath, qreal opacity)
{
    QPixmap pm(iconPath);
    if (pm.isNull())
        return QIcon(iconPath);

    QPixmap out(pm.size());
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setOpacity(opacity);
    p.drawPixmap(0, 0, pm);
    p.end();
    return QIcon(out);
}

void Header::refreshPropertiesBtnState()
{
    if (!sourcePropertiesPanelBtn)
        return;

    obs_source_t *src = model->selectedSource();
    const bool sceneOrGroup =
        src && obs_group_or_scene_from_source(src) != nullptr;

    sourcePropertiesPanelBtn->setEnabled(!sceneOrGroup);

    const QString propPath = QString::fromUtf8(
        editorui::GetIconPath("Properties.svg").c_str());
    sourcePropertiesPanelBtn->setIcon(
        sceneOrGroup ? AtenuatedIcon(propPath, 0.5)
                     : QIcon(propPath));

    if (activePanelIndex == 1)
        setButtonRed(sourcePropertiesPanelBtn);
    else
        setButtonGreen(sourcePropertiesPanelBtn);

    if (sceneOrGroup && activePanelIndex == 1 && sourceIconBtn)
        sourceIconBtn->click();
}

static QIcon PreviewBtnIcon(const QColor *tint)
{
    const QString path = QString::fromUtf8(
        editorui::GetIconPath("Output.svg").c_str());

    if (!QFile::exists(path))
        return QIcon();

    if (!tint)
        return QIcon(path);

    QPixmap pm(path);
    if (pm.isNull())
        return QIcon(path);

    QPixmap out(pm.size());
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.drawPixmap(0, 0, pm);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(out.rect(), *tint);
    p.end();
    return QIcon(out);
}

void Header::applyPreviewBtnIcon()
{
    if (!previewBtn || previewState == 0)
        return;

    if (previewState == 2) {
        const QColor yellow(0xe0, 0xb3, 0x5e);
        previewBtn->setIcon(PreviewBtnIcon(&yellow));
    } else {
        previewBtn->setIcon(PreviewBtnIcon(nullptr));
    }
}

void Header::refreshPreviewBtn()
{
    if (!previewBtn || !previewBlink)
        return;

    obs_source_t *src = model ? model->selectedSource() : nullptr;
    const bool hasVideo =
        src && (obs_source_get_output_flags(src) & OBS_SOURCE_VIDEO);

    if (previewWin && src && hasVideo)
        previewWin->setSource(src);

    if (!hasVideo && previewWin && previewWin->isOpen()) {
        if (!previewAutoClose->isActive())
            previewAutoClose->start();
    } else if (previewAutoClose) {
        previewAutoClose->stop();
    }

    int wanted;
    if (!hasVideo)
        wanted = 1;
    else if (previewWin && previewWin->isOpen())
        wanted = 3;
    else
        wanted = 2;

    if (wanted == previewState)
        return;

    previewState = wanted;

    switch (wanted) {
    case 1:
        previewBlink->stop();
        previewBtn->setStyleSheet(QString());
        applyPreviewBtnIcon();
        previewBtn->setEnabled(false);
        break;
    case 2:
        previewBtn->setEnabled(true);
        applyPreviewBtnIcon();
        previewBlink->start();
        break;
    case 3:
        previewBlink->stop();
        previewBtn->setEnabled(true);
        previewBtn->setStyleSheet(
            QStringLiteral("QPushButton { border: 2px solid #FF4444; }"));
        applyPreviewBtnIcon();
        break;
    }
}

void Header::renameScene()
{
    obs_source_t *sceneSrc = model->sceneSourcePtr();
    if (!sceneSrc)
        return;

    QString newName = sceneCombo->currentText();
    obs_source_set_name(sceneSrc, newName.toUtf8().constData());

    sceneCombo->clearFocus();
    sceneCombo->lineEdit()->clearFocus();
}

void Header::renameSource()
{
    obs_source_t *src = model->currentSourcePtr();
    if (!src)
        return;

    QString newName = sourceCombo->currentText();
    obs_source_set_name(src, newName.toUtf8().constData());

    sourceCombo->clearFocus();
    sourceCombo->lineEdit()->clearFocus();
}

void Header::setButtonRed(QPushButton *btn)
{
    if (!btn) return;
    btn->setStyleSheet(btn->isEnabled()
                           ? "QPushButton { border: 2px solid #FF4444; }"
                           : "QPushButton { border: 1px solid palette(button); }");
}

void Header::setButtonGreen(QPushButton *btn)
{
    if (!btn) return;
    btn->setStyleSheet(btn->isEnabled()
                           ? "QPushButton { border: 2px solid #16c416; }"
                           : "QPushButton { border: 1px solid palette(button); }");
}

   void Header::updatePanelButtonStyles(int index)
{
    activePanelIndex = index;

    setButtonGreen(sceneFilterPanelBtn);
    setButtonGreen(sceneNotesBtn);
    setButtonGreen(sourceIconBtn);
    setButtonGreen(sourcePropertiesPanelBtn);
    setButtonGreen(sourceTransformPanelBtn);
    setButtonGreen(sourceFiltersPanelBtn);
    setButtonGreen(sourceNotesBtn);

    switch (index) {
        case 0: setButtonRed(sourceIconBtn); break;
        case 1: setButtonRed(sourcePropertiesPanelBtn); break;
        case 2: setButtonRed(sourceFiltersPanelBtn); break;
        case 3: setButtonRed(sceneFilterPanelBtn); break;
        case 4: setButtonRed(sourceTransformPanelBtn); break;
        case 5: setButtonRed(sceneNotesBtn); break;
        case 6: setButtonRed(sourceNotesBtn); break;
    }
}