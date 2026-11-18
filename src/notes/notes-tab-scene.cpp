#include "notes-tab-scene.hpp"
#include "editor-model.hpp"
#include "edit-note-bar.hpp"
#include "menu-context.hpp"

#include <obs-module.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QSaveFile>
#include <QShortcut>
#include <QSignalBlocker>
#include <QTabBar>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

static void loadNoteText(QTextEdit *ed, const QString &text)
{
    const QString t = text.trimmed();
    if (t.startsWith(QLatin1String("<!DOCTYPE")) ||
        t.startsWith(QLatin1String("<html")) ||
        t.contains(QLatin1String("<meta name=\"qrichtext\"")))
        ed->setHtml(text);
    else
        ed->setPlainText(text);
}

SceneNotesTab::SceneNotesTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    auto *topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(4);

    bar = new QTabBar(this);
    bar->setObjectName("notesTabBar");
    bar->setTabsClosable(false);
    bar->setExpanding(false);
    topRow->addWidget(bar);

    addBtn = new QPushButton(this);
    addBtn->setObjectName("notesAddTabButton");
    addBtn->setProperty("class", "icon-plus");
    addBtn->setIconSize(QSize(14, 14));
    addBtn->setFixedSize(18, 18);
    addBtn->setStyleSheet(QStringLiteral(
        "#notesAddTabButton { height: 18px; max-height: 18px;"
        " min-height: 18px; margin: 0px; padding: 0px; }"));
    topRow->addWidget(addBtn);

    delBtn = new QPushButton(this);
    delBtn->setObjectName("notesDeleteTabButton");
    delBtn->setProperty("class", "icon-trash");
    delBtn->setIconSize(QSize(14, 14));
    delBtn->setFixedSize(18, 18);
    delBtn->setStyleSheet(QStringLiteral(
        "#notesDeleteTabButton { height: 18px; max-height: 18px;"
        " min-height: 18px; margin: 0px; padding: 0px; }"));
    topRow->addWidget(delBtn);
    topRow->addStretch();

    layout->addLayout(topRow);

    editor = new QTextEdit(this);
    editor->setObjectName("notesEditor");
    editor->setAcceptRichText(true);
    editor->setPlaceholderText(obs_module_text("NotesPlaceholder"));
    editor->setSizePolicy(editor->sizePolicy().horizontalPolicy(),
                          QSizePolicy::Expanding);
    layout->addWidget(editor);

    editActions = new NoteEditActions(editor, this);
    editBar = new EditNoteBar(editActions, this);
    layout->insertWidget(0, editBar);

    editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(editor, &QWidget::customContextMenuRequested, this,
            [this](const QPoint &pos) {
                MenuContext menu(editActions, this);
                menu.exec(editor->mapToGlobal(pos));
            });

    load();

    saveTimer = new QTimer(this);
    saveTimer->setSingleShot(true);
    saveTimer->setInterval(400);
    connect(saveTimer, &QTimer::timeout, this, [this]() { flush(); });

    connect(editor, &QTextEdit::textChanged, this, [this]() {
        if (loading || currentUuid.isEmpty())
            return;
        const int i = bar->currentIndex();
        if (i < 0 || i >= int(tabs.size()))
            return;
        tabs[i].text = editor->toHtml();
        uiDirty = fileDirty = true;
        scheduleSave();
    });

    connect(bar, &QTabBar::currentChanged,
            this, &SceneNotesTab::onBarCurrentChanged);
    connect(bar, &QTabBar::tabCloseRequested,
            this, &SceneNotesTab::onTabCloseRequested);
    connect(bar, &QTabBar::tabBarDoubleClicked,
            this, &SceneNotesTab::startRename);
    connect(addBtn, &QPushButton::clicked,
            this, &SceneNotesTab::onAddTab);
    connect(delBtn, &QPushButton::clicked, this, [this]() {
        const int idx = bar->currentIndex();
        if (idx >= 0)
            onTabCloseRequested(idx);
    });

    connect(model, &EditorModel::sceneChanged,
            this, &SceneNotesTab::refresh);
    connect(model, &EditorModel::modelUpdated,
            this, &SceneNotesTab::refresh);

    refresh();
}

SceneNotesTab::~SceneNotesTab()
{
    flush();
}

QString SceneNotesTab::filePath() const
{
    const QString base = qEnvironmentVariable("APPDATA");
    const QString dir = base.isEmpty()
        ? QDir::homePath() + QStringLiteral("/E33-Editor")
        : base + QStringLiteral("/E33-Editor");
    return dir + QStringLiteral("/notes-scenes.json");
}

void SceneNotesTab::load()
{
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (doc.isObject())
        data = doc.object();
}

void SceneNotesTab::scheduleSave()
{
    saveTimer->start();
}

void SceneNotesTab::loadEntity(const QString &uuid)
{
    tabs.clear();
    int active = 0;
    const QJsonValue v = data.value(uuid);

    if (v.isString()) {
        NoteTab nt;
        nt.name = defaultTabName();
        nt.text = v.toString();
        tabs.append(nt);

        QJsonArray arr;
        QJsonObject o;
        o.insert(QStringLiteral("name"), nt.name);
        o.insert(QStringLiteral("text"), nt.text);
        arr.append(o);
        QJsonObject obj;
        obj.insert(QStringLiteral("tabs"), arr);
        obj.insert(QStringLiteral("active"), 0);
        data.insert(uuid, obj);
        fileDirty = true;
    } else if (v.isObject()) {
        const QJsonObject o = v.toObject();
        const QJsonArray arr = o.value(QStringLiteral("tabs")).toArray();
        for (const QJsonValue &e : arr) {
            const QJsonObject eo = e.toObject();
            NoteTab nt;
            nt.name = eo.value(QStringLiteral("name")).toString();
            nt.text = eo.value(QStringLiteral("text")).toString();
            if (nt.name.isEmpty())
                nt.name = defaultTabName();
            tabs.append(nt);
        }
        active = o.value(QStringLiteral("active")).toInt(0);
    }

    if (tabs.isEmpty())
        tabs.append({defaultTabName(), QString()});
    if (active < 0 || active >= int(tabs.size()))
        active = 0;

    clearBar();
    for (const NoteTab &nt : tabs)
        bar->addTab(nt.name);
    bar->setCurrentIndex(active);

    loadNoteText(editor, tabs[active].text);
}

QJsonObject SceneNotesTab::serializeEntity() const
{
    QJsonArray arr;
    for (const NoteTab &nt : tabs) {
        QJsonObject o;
        o.insert(QStringLiteral("name"), nt.name);
        o.insert(QStringLiteral("text"), nt.text);
        arr.append(o);
    }
    QJsonObject obj;
    obj.insert(QStringLiteral("tabs"), arr);
    obj.insert(QStringLiteral("active"), bar->currentIndex());
    return obj;
}

QString SceneNotesTab::defaultTabName() const
{
    return QString(obs_module_text("NotesTabNew")).arg(1);
}

void SceneNotesTab::flush()
{
    if (uiDirty && !currentUuid.isEmpty()) {
        data.insert(currentUuid, serializeEntity());
        uiDirty = false;
        fileDirty = true;
    }
    if (!fileDirty)
        return;

    QDir().mkpath(QFileInfo(filePath()).absolutePath());

    QSaveFile f(filePath());
    if (!f.open(QIODevice::WriteOnly))
        return;

    f.write(QJsonDocument(data).toJson(QJsonDocument::Indented));
    if (f.commit())
        fileDirty = false;
}

QString SceneNotesTab::uuidForCurrent() const
{
    obs_source_t *src = model ? model->sceneSourcePtr() : nullptr;
    if (!src)
        return QString();

    const char *uuid = obs_source_get_uuid(src);
    return uuid ? QString::fromUtf8(uuid) : QString();
}

void SceneNotesTab::refresh()
{
    const QString uuid = uuidForCurrent();
    if (refreshed && uuid == currentUuid)
        return;
    refreshed = true;
    cancelRename();

    if (!currentUuid.isEmpty() && uiDirty) {
        data.insert(currentUuid, serializeEntity());
        uiDirty = false;
        fileDirty = true;
    }
    currentUuid = uuid;

    loading = true;
    const QSignalBlocker barBlock(bar);
    const QSignalBlocker edBlock(editor);

    if (uuid.isEmpty()) {
        tabs.clear();
        clearBar();
        editor->clear();
        editor->setEnabled(false);
        editBar->setEnabled(false);
        addBtn->setEnabled(false);
        editor->setPlaceholderText(obs_module_text("NoSourceSelected"));
    } else {
        loadEntity(uuid);
        editor->setEnabled(true);
        editBar->setEnabled(true);
        addBtn->setEnabled(true);
        editor->setPlaceholderText(obs_module_text("NotesPlaceholder"));
    }

    loading = false;

    if (fileDirty)
        scheduleSave();
}

void SceneNotesTab::onBarCurrentChanged(int index)
{
    if (loading || currentUuid.isEmpty() ||
        index < 0 || index >= int(tabs.size()))
        return;

    loading = true;
    const QSignalBlocker blocker(editor);
    loadNoteText(editor, tabs[index].text);
    loading = false;

    editor->clearFocus();

    uiDirty = fileDirty = true;
    scheduleSave();
}

void SceneNotesTab::onTabCloseRequested(int index)
{
    if (loading || currentUuid.isEmpty())
        return;
    if (tabs.size() <= 1)
        return;

    tabs.removeAt(index);

    int active = bar->currentIndex();
    if (index < active)
        --active;
    else if (index == active)
        active = qBound(0, active, int(tabs.size()) - 1);

    loading = true;
    {
        const QSignalBlocker blocker(bar);
        bar->removeTab(index);
        bar->setCurrentIndex(active);
    }
    {
        const QSignalBlocker blocker(editor);
        loadNoteText(editor, tabs[active].text);
    }
    loading = false;

    uiDirty = fileDirty = true;
    scheduleSave();
}

void SceneNotesTab::onAddTab()
{
    if (currentUuid.isEmpty())
        return;

    NoteTab nt;
    nt.name = QString(obs_module_text("NotesTabNew"))
                  .arg(int(tabs.size()) + 1);
    tabs.append(nt);

    const int idx = int(tabs.size()) - 1;
    bar->addTab(nt.name);
    bar->setCurrentIndex(idx);

    uiDirty = fileDirty = true;
    scheduleSave();
}

void SceneNotesTab::clearBar()
{
    while (bar->count() > 0)
        bar->removeTab(bar->count() - 1);
}

void SceneNotesTab::cancelRename()
{
    if (!renameEdit)
        return;
    QLineEdit *ed = renameEdit;
    renameEdit = nullptr;
    delete ed;
}

void SceneNotesTab::startRename(int index)
{
    if (loading || currentUuid.isEmpty() || renameEdit)
        return;
    if (index < 0 || index >= bar->count())
        return;

    const QString original = bar->tabText(index);

    renameEdit = new QLineEdit(bar);
    renameEdit->setObjectName("notesRenameEdit");
    renameEdit->setText(original);
    renameEdit->selectAll();
    renameEdit->setFrame(false);
    renameEdit->setGeometry(bar->tabRect(index).adjusted(2, 2, -2, -2));

    connect(renameEdit, &QLineEdit::editingFinished, this,
            [this, index, original]() {
                commitRename(index, original);
            });
    auto *esc = new QShortcut(QKeySequence(Qt::Key_Escape), renameEdit);
    esc->setContext(Qt::WidgetShortcut);
    connect(esc, &QShortcut::activated, this, [this]() {
        cancelRename();
    });

    renameEdit->show();
    renameEdit->setFocus();
}

void SceneNotesTab::commitRename(int index, const QString &original)
{
    if (!renameEdit)
        return;
    const QString text = renameEdit->text();
    cancelRename();

    if (loading || index < 0 || index >= int(tabs.size()))
        return;
    if (bar->tabText(index) != original)
        return;

    const QString name =
        text.trimmed().isEmpty() ? original : text.trimmed();
    if (name == tabs[index].name)
        return;

    tabs[index].name = name;
    bar->setTabText(index, name);
    uiDirty = fileDirty = true;
    scheduleSave();
}

void SceneNotesTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && editor->hasFocus())
        editor->clearFocus();
    QWidget::mousePressEvent(event);
}
