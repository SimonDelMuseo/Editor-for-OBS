#include "edit-note-bar.hpp"

#include <obs-module.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QCoreApplication>
#include <QEvent>
#include <QFontDialog>
#include <QFrame>
#include <QIcon>
#include <QKeySequence>
#include <QLayout>
#include <QLayoutItem>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextFormat>
#include <QTextList>
#include <QTextListFormat>
#include <QVector>

namespace
{

QString T(const char *key)
{
    return QString::fromUtf8(obs_module_text(key));
}

QString accel(QKeySequence::StandardKey key)
{
    if (QCoreApplication::testAttribute(
            Qt::AA_DontShowShortcutsInContextMenus))
        return QString();
    return QLatin1Char('\t') +
           QKeySequence(key).toString(QKeySequence::NativeText);
}

const QTextListFormat::Style kListStyles[8] = {
    QTextListFormat::ListDisc,
    QTextListFormat::ListCircle,
    QTextListFormat::ListSquare,
    QTextListFormat::ListDecimal,
    QTextListFormat::ListLowerAlpha,
    QTextListFormat::ListUpperAlpha,
    QTextListFormat::ListLowerRoman,
    QTextListFormat::ListUpperRoman,
};
const char *const kListKeys[8] = {
    "NotesListDisc",       "NotesListCircle",       "NotesListSquare",
    "NotesListDecimal",    "NotesListLowerAlpha",   "NotesListUpperAlpha",
    "NotesListLowerRoman", "NotesListUpperRoman",
};

QIcon letterIcon(const QString &letter, const QColor &letterColor,
                 const QColor &bgColor)
{
    const int px = 18;
    QIcon icon;
    for (int dpr = 1; dpr <= 2; ++dpr) {
        QPixmap pm(px * dpr, px * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        if (bgColor.isValid()) {
            p.setPen(Qt::NoPen);
            p.setBrush(bgColor);
            p.drawRoundedRect(
                QRectF(1 * dpr, 1 * dpr, (px - 2) * dpr, (px - 2) * dpr),
                2.0 * dpr, 2.0 * dpr);
        }
        QColor lc = letterColor.isValid() ? letterColor : QColor(0, 0, 0);
        if (bgColor.isValid()) {
            const int lum = int(0.299 * bgColor.red() +
                                0.587 * bgColor.green() +
                                0.114 * bgColor.blue());
            lc = lum > 140 ? QColor(0, 0, 0) : QColor(255, 255, 255);
        }
        QFont f;
        f.setBold(true);
        f.setPixelSize(17 * dpr);
        p.setFont(f);
        p.setPen(lc);
        p.drawText(QRectF(0, 0, px * dpr, px * dpr),
                   Qt::AlignCenter, letter);
        icon.addPixmap(pm);
    }
    return icon;
}

QIcon listIcon(const QColor &c)
{
    const int px = 18;
    QIcon icon;
    for (int dpr = 1; dpr <= 2; ++dpr) {
        QPixmap pm(px * dpr, px * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        const qreal rows[3] = {4.0, 9.0, 14.0};
        for (const qreal cy : rows) {
            p.drawEllipse(QRectF(1.7 * dpr, (cy - 1.7) * dpr,
                                 3.4 * dpr, 3.4 * dpr));
            p.drawRoundedRect(QRectF(6.5 * dpr, (cy - 1.5) * dpr,
                                     10.0 * dpr, 3.0 * dpr),
                              1.5 * dpr, 1.5 * dpr);
        }
        icon.addPixmap(pm);
    }
    return icon;
}

class BarFlowLayout : public QLayout
{
public:
    explicit BarFlowLayout(QWidget *parent) : QLayout(parent) {}

    void addItem(QLayoutItem *item) override { m_items.append(item); }
    int count() const override { return m_items.size(); }
    QLayoutItem *itemAt(int i) const override { return m_items.value(i); }
    QLayoutItem *takeAt(int i) override
    {
        return (i >= 0 && i < m_items.size()) ? m_items.takeAt(i)
                                              : nullptr;
    }

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override
    {
        return doLayout(QRect(0, 0, w, 0), false);
    }

    QSize sizeHint() const override
    {
        return QSize(singleRowWidth(), doLayout(
                         QRect(0, 0, 1000000, 0), false));
    }

    QSize minimumSize() const override
    {
        int maxW = 0;
        for (QLayoutItem *it : m_items)
            maxW = qMax(maxW, it->sizeHint().width());
        return QSize(maxW + contentsMargins().left() +
                         contentsMargins().right(),
                     doLayout(QRect(0, 0, 1000000, 0), false));
    }

    void setGeometry(const QRect &rect) override
    {
        QLayout::setGeometry(rect);
        doLayout(rect, true);
    }

private:
    int singleRowWidth() const
    {
        int w = 0;
        for (int i = 0; i < m_items.size(); ++i)
            w += m_items.at(i)->sizeHint().width() +
                 (i > 0 ? qMax(0, spacing()) : 0);
        return w + contentsMargins().left() + contentsMargins().right();
    }

    int doLayout(const QRect &rect, bool place) const
    {
        const QMargins m = contentsMargins();
        const int sp = qMax(0, spacing());
        const int avail = rect.width() - m.left() - m.right();

        struct Entry { QLayoutItem *item; int w; int h; };
        QVector<Entry> row;
        int rowW = 0;
        int rowH = 0;
        int y = rect.y() + m.top();

        auto flushRow = [&]() {
            if (row.isEmpty())
                return;
            const int startX =
                rect.x() + m.left() + qMax(0, (avail - rowW) / 2);
            if (place) {
                int cx = startX;
                for (const Entry &e : row) {
                    e.item->setGeometry(
                        QRect(cx, y + (rowH - e.h) / 2, e.w, e.h));
                    cx += e.w + sp;
                }
            }
            y += rowH;
            row.clear();
            rowW = 0;
            rowH = 0;
        };

        for (QLayoutItem *it : m_items) {
            const QSize sh = it->sizeHint();
            if (!row.isEmpty() && rowW + sp + sh.width() > avail)
                flushRow();
            const int gap = row.isEmpty() ? 0 : sp;
            row.append({it, sh.width(), sh.height()});
            rowW += gap + sh.width();
            rowH = qMax(rowH, sh.height());
        }
        flushRow();

        return y + m.bottom() - rect.y();
    }

    QList<QLayoutItem *> m_items;
};

}

NoteEditActions::NoteEditActions(QTextEdit *editor, QObject *parent)
    : QObject(parent)
    , m_editor(editor)
{
    build();
    refresh();
    updateColorIcons();
}

void NoteEditActions::build()
{
    m_a.undo = new QAction(T("NotesUndo") + accel(QKeySequence::Undo), this);
    m_a.undo->setObjectName(QStringLiteral("edit-undo"));
    registerThemeIcon(m_a.undo, "edit-undo");
    connect(m_a.undo, &QAction::triggered, m_editor, &QTextEdit::undo);

    m_a.redo = new QAction(T("NotesRedo") + accel(QKeySequence::Redo), this);
    m_a.redo->setObjectName(QStringLiteral("edit-redo"));
    registerThemeIcon(m_a.redo, "edit-redo");
    connect(m_a.redo, &QAction::triggered, m_editor, &QTextEdit::redo);

    m_a.cut = new QAction(T("NotesCut") + accel(QKeySequence::Cut), this);
    m_a.cut->setObjectName(QStringLiteral("edit-cut"));
    registerThemeIcon(m_a.cut, "edit-cut");
    connect(m_a.cut, &QAction::triggered, m_editor, &QTextEdit::cut);

    m_a.copy = new QAction(T("NotesCopy") + accel(QKeySequence::Copy), this);
    m_a.copy->setObjectName(QStringLiteral("edit-copy"));
    registerThemeIcon(m_a.copy, "edit-copy");
    connect(m_a.copy, &QAction::triggered, m_editor, &QTextEdit::copy);

    m_a.paste =
        new QAction(T("NotesPaste") + accel(QKeySequence::Paste), this);
    m_a.paste->setObjectName(QStringLiteral("edit-paste"));
    registerThemeIcon(m_a.paste, "edit-paste");
    connect(m_a.paste, &QAction::triggered, m_editor, &QTextEdit::paste);

    m_a.selectAll = new QAction(
        T("NotesSelectAll") + accel(QKeySequence::SelectAll), this);
    m_a.selectAll->setObjectName(QStringLiteral("select-all"));
    registerThemeIcon(m_a.selectAll, "edit-select-all");
    connect(m_a.selectAll, &QAction::triggered, m_editor,
            &QTextEdit::selectAll);

    m_a.font = new QAction(T("NotesFont"), this);
    registerThemeIcon(m_a.font, "preferences-desktop-font");
    connect(m_a.font, &QAction::triggered, this, [this]() {
        bool ok = false;
        const QFont f = QFontDialog::getFont(&ok, m_editor->currentFont(),
                                             m_editor, T("NotesFont"));
        if (ok)
            m_editor->setCurrentFont(f);
    });

    m_a.textColor = new QAction(T("NotesTextColor"), this);
    connect(m_a.textColor, &QAction::triggered, this, [this]() {
        const QColor c = QColorDialog::getColor(m_editor->textColor(),
                                                m_editor,
                                                T("NotesTextColor"));
        if (c.isValid())
            m_editor->setTextColor(c);
    });

    m_a.backgroundColor = new QAction(T("NotesBackgroundColor"), this);
    connect(m_a.backgroundColor, &QAction::triggered, this, [this]() {
        const QColor c = QColorDialog::getColor(
            m_editor->textBackgroundColor(), m_editor,
            T("NotesBackgroundColor"));
        if (c.isValid())
            m_editor->setTextBackgroundColor(c);
    });
    m_a.backgroundColor->setIcon(
        letterIcon(QStringLiteral("A"), QColor(), QColor(128, 128, 128)));

    for (int i = 0; i < 8; ++i) {
        QAction *la = new QAction(T(kListKeys[i]), this);
        la->setCheckable(true);
        const QTextListFormat::Style st = kListStyles[i];
        connect(la, &QAction::triggered, this, [this, st]() {
            QTextCursor c = m_editor->textCursor();
            c.beginEditBlock();
            if (QTextList *l = c.currentList()) {
                QTextListFormat f = l->format();
                f.setStyle(st);
                l->setFormat(f);
            } else {
                c.createList(st);
            }
            c.endEditBlock();
            refresh();
        });
        m_a.list.append(la);
    }

    m_a.indentIncrease = new QAction(T("NotesIndentIncrease"), this);
    registerThemeIcon(m_a.indentIncrease, "format-indent-more");
    connect(m_a.indentIncrease, &QAction::triggered, this, [this]() {
        QTextCursor c = m_editor->textCursor();
        QTextBlockFormat bf = c.block().blockFormat();
        bf.setIndent(bf.indent() + 1);
        c.beginEditBlock();
        c.setBlockFormat(bf);
        c.endEditBlock();
    });

    m_a.indentDecrease = new QAction(T("NotesIndentDecrease"), this);
    registerThemeIcon(m_a.indentDecrease, "format-indent-less");
    connect(m_a.indentDecrease, &QAction::triggered, this, [this]() {
        QTextCursor c = m_editor->textCursor();
        const QTextBlockFormat bf = c.block().blockFormat();
        c.beginEditBlock();
        if (bf.indent() > 0) {
            QTextBlockFormat f = bf;
            f.setIndent(bf.indent() - 1);
            c.setBlockFormat(f);
        } else if (QTextList *l = c.block().textList()) {
            const int n = l->count();
            for (int i = 0; i < n; ++i)
                l->removeItem(0);
        }
        c.endEditBlock();
    });

    m_a.clearFormat = new QAction(T("NotesClearFormat"), this);
    registerThemeIcon(m_a.clearFormat, "edit-clear");
    connect(m_a.clearFormat, &QAction::triggered, this, [this]() {
        const QString text = m_editor->toPlainText();
        m_editor->setTextColor(
            m_editor->palette().color(QPalette::ColorRole::Text));
        m_editor->setTextBackgroundColor(
            m_editor->palette().color(QPalette::ColorRole::Base));
        m_editor->setCurrentFont(m_editor->font());
        m_editor->setPlainText(text);
    });

    const QList<QAction *> all = {
        m_a.undo,       m_a.redo,          m_a.cut,
        m_a.copy,       m_a.paste,         m_a.selectAll,
        m_a.font,       m_a.textColor,     m_a.backgroundColor,
        m_a.clearFormat, m_a.indentIncrease, m_a.indentDecrease,
    };
    for (QAction *a : all) {
        QString tip = a->text();
        tip.replace(QLatin1Char('\t'), QStringLiteral("    "));
        a->setToolTip(tip);
    }
    for (QAction *la : std::as_const(m_a.list))
        la->setToolTip(la->text());

    connect(m_editor->document(), &QTextDocument::undoAvailable,
            this, &NoteEditActions::refresh);
    connect(m_editor->document(), &QTextDocument::redoAvailable,
            this, &NoteEditActions::refresh);
    connect(m_editor, &QTextEdit::textChanged,
            this, &NoteEditActions::refresh);
    connect(m_editor, &QTextEdit::selectionChanged,
            this, &NoteEditActions::refresh);
    connect(m_editor, &QTextEdit::cursorPositionChanged,
            this, &NoteEditActions::refresh);
    connect(m_editor, &QTextEdit::currentCharFormatChanged, this,
            [this](const QTextCharFormat &) {
                refresh();
                updateColorIcons();
            });
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &NoteEditActions::refresh);
}

void NoteEditActions::registerThemeIcon(QAction *action,
                                        const char *themeName)
{
    const QString name = QString::fromUtf8(themeName);
    m_themeIcons.append(qMakePair(action, name));
    const QIcon icon = QIcon::fromTheme(name);
    if (!icon.isNull())
        action->setIcon(icon);
}

void NoteEditActions::applyThemeIcons()
{
    for (const auto &entry : std::as_const(m_themeIcons)) {
        const QIcon icon = QIcon::fromTheme(entry.second);
        if (!icon.isNull())
            entry.first->setIcon(icon);
    }
}

void NoteEditActions::refresh()
{
    if (!m_editor)
        return;

    const bool ro = m_editor->isReadOnly();
    const bool sel = m_editor->textCursor().hasSelection();

    m_a.undo->setEnabled(!ro && m_editor->document()->isUndoAvailable());
    m_a.redo->setEnabled(!ro && m_editor->document()->isRedoAvailable());
    m_a.cut->setEnabled(!ro && sel);
    m_a.copy->setEnabled(sel);

    const QMimeData *mime = QApplication::clipboard()->mimeData();
    m_a.paste->setEnabled(!ro && mime &&
                          (mime->hasText() || mime->hasHtml()));

    m_a.selectAll->setEnabled(!m_editor->document()->isEmpty());

    QTextCursor c = m_editor->textCursor();
    const QTextList *l = c.currentList();
    const QTextListFormat::Style cur =
        l ? l->format().style() : QTextListFormat::Style(0);
    for (int i = 0; i < m_a.list.size(); ++i)
        m_a.list[i]->setChecked(kListStyles[i] == cur);
}

void NoteEditActions::updateColorIcons()
{
    if (!m_editor || !m_a.textColor)
        return;

    QColor tc = m_editor->textColor();
    if (!tc.isValid())
        tc = m_editor->palette().color(QPalette::Text);
    m_a.textColor->setIcon(letterIcon(QStringLiteral("A"), tc, QColor()));

}

EditNoteBar::EditNoteBar(NoteEditActions *acts, QWidget *parent)
    : QWidget(parent)
    , m_acts(acts)
{
    setObjectName(QStringLiteral("notesEditBar"));

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto *lay = new BarFlowLayout(this);
    lay->setContentsMargins(14, 1, 14, 7);
    lay->setSpacing(6);

    const NoteActions &a = acts->actions();

    auto addBtn = [this, lay](QAction *act, int iconPx) {
        auto *b = new QPushButton(this);
        b->setFixedSize(26, 26);
        b->setIconSize(QSize(iconPx, iconPx));
        auto sync = [b, act]() {
            b->setIcon(act->icon());
            b->setEnabled(act->isEnabled());
            b->setToolTip(act->toolTip());
        };
        sync();
        connect(act, &QAction::changed, b, sync);
        connect(b, &QPushButton::clicked, act, &QAction::trigger);
        lay->addWidget(b);
    };

    auto addSep = [this, lay]() {
        auto *sep = new QFrame(this);
        sep->setFrameShape(QFrame::VLine);
        sep->setFrameShadow(QFrame::Plain);
        sep->setFixedHeight(18);
        lay->addWidget(sep);
    };

    addBtn(a.undo, 14);
    addBtn(a.redo, 14);
    addSep();
    addBtn(a.cut, 14);
    addBtn(a.copy, 14);
    addBtn(a.paste, 14);
    addBtn(a.selectAll, 14);
    addSep();
    addBtn(a.font, 14);
    addSep();
    addBtn(a.textColor, 18);
    addBtn(a.backgroundColor, 18);
    addSep();

    m_listBtn = new QPushButton(this);
    m_listBtn->setObjectName(QStringLiteral("notesListButton"));
    m_listBtn->setToolTip(T("NotesList"));
    m_listBtn->setFixedSize(26, 26);
    m_listBtn->setIconSize(QSize(18, 18));
    auto *listMenu = new QMenu(m_listBtn);
    for (QAction *la : a.list)
        listMenu->addAction(la);
    listMenu->addSeparator();
    listMenu->addAction(a.indentIncrease);
    listMenu->addAction(a.indentDecrease);
    connect(m_listBtn, &QPushButton::clicked, this, [this, listMenu]() {
        listMenu->popup(
            m_listBtn->mapToGlobal(QPoint(0, m_listBtn->height())));
    });
    lay->addWidget(m_listBtn);
    m_listBtn->setIcon(listIcon(palette().color(QPalette::Text)));

    addSep();
    addBtn(a.clearFormat, 18);

}

void EditNoteBar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::StyleChange:
    case QEvent::PaletteChange:
    case QEvent::ThemeChange:
        m_acts->applyThemeIcons();
        m_acts->updateColorIcons();
        if (m_listBtn)
            m_listBtn->setIcon(listIcon(palette().color(QPalette::Text)));
        break;
    default:
        break;
    }
}

void EditNoteBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    const int want = heightForWidth(width());
    if (want > 0 && maximumHeight() != want)
        setMaximumHeight(want);
    blog(LOG_INFO, "[E33][notabar] w=%d h=%d hfw=%d maxH=%d pageH=%d",
         width(), height(), want, maximumHeight(),
         parentWidget() ? parentWidget()->height() : -1);
}
