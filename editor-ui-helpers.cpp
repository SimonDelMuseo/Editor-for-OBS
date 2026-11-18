#include "editor-ui-helpers.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QIcon>
#include <QString>
#include <QDir>
#include <QMainWindow>
#include <QAction>
#include <QMetaObject>
#include <QAbstractButton>
#include <QFileInfo>
#include <QPointer>
#include <QVector>
#include <QApplication>
#include <QPalette>
#include <QRegularExpression>

namespace editorui {

std::string GetThemeTypeName()
{
    return obs_frontend_is_theme_dark() ? "Dark" : "Light";
}

namespace {
struct RegisteredIcon {
    QPointer<QAbstractButton> button;
    QString iconName;
};

QVector<RegisteredIcon> &IconRegistry()
{
    static QVector<RegisteredIcon> registry;
    return registry;
}
}

void SetButtonIcon(QAbstractButton *button, const char *path)
{
    QIcon icon;
    icon.addFile(QString::fromUtf8(path), QSize(), QIcon::Normal, QIcon::Off);
    if (button)
        button->setIcon(icon);

    if (!button)
        return;

    const QString fileName =
        QFileInfo(QString::fromUtf8(path)).fileName();
    QString iconName;
    if (fileName.startsWith(QLatin1String("Dark")))
        iconName = fileName.mid(4);
    else if (fileName.startsWith(QLatin1String("Light")))
        iconName = fileName.mid(5);
    if (iconName.isEmpty())
        return;

    for (RegisteredIcon &entry : IconRegistry()) {
        if (entry.button == button) {
            entry.iconName = iconName;
            return;
        }
    }
    IconRegistry().append(
        RegisteredIcon{QPointer<QAbstractButton>(button), iconName});
}

std::string GetIconPath(const std::string &iconName)
{
    const std::string theme = GetThemeTypeName();
    return ":/icons/" + theme + iconName;
}

void RefreshButtonIcons()
{
    for (qsizetype i = IconRegistry().size() - 1; i >= 0; --i) {
        RegisteredIcon &entry = IconRegistry()[i];
        if (!entry.button) {
            IconRegistry().removeAt(i);
            continue;
        }

        const std::string path =
            GetIconPath(entry.iconName.toStdString());
        QIcon icon;
        icon.addFile(QString::fromUtf8(path), QSize(),
                     QIcon::Normal, QIcon::Off);
        entry.button->setIcon(icon);
    }
}

QAction *FindNativeAction(const char *name)
{
    auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!mainWindow)
        return nullptr;

    return mainWindow->findChild<QAction *>(name);
}

bool TriggerNativeAction(const char *name)
{
    QAction *action = FindNativeAction(name);
    if (!action)
        return false;

    if (action->isEnabled())
        action->trigger();

    return true;
}

bool InvokeNativeMethod(const char *method)
{
    auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!mainWindow)
        return false;

    return QMetaObject::invokeMethod(mainWindow, method, Qt::DirectConnection);
}


namespace {

QString resolveVar(const QString &sheet, const QString &name, int depth = 0)
{
    if (depth > 8)
        return QString();

    const QRegularExpression def(
        QRegularExpression::escape(name) + QStringLiteral("\\s*:\\s*([^;]+);"));
    const QRegularExpressionMatch m = def.match(sheet);
    if (!m.hasMatch())
        return QString();

    QString value = m.captured(1).trimmed();

    const QRegularExpression use(
        QStringLiteral("var\\(\\s*(--[\\w-]+)\\s*\\)"));
    QRegularExpressionMatch vm = use.match(value);
    while (vm.hasMatch()) {
        const QString inner = resolveVar(sheet, vm.captured(1), depth + 1);
        if (inner.isEmpty()) {
            value.clear();
            break;
        }
        value.replace(vm.capturedStart(), vm.capturedLength(), inner);
        vm = use.match(value);
    }
    return value.trimmed();
}

QColor toColor(const QString &sheet, const QString &value)
{
    static const QRegularExpression pal(
        QStringLiteral("^palette\\(\\s*(\\w+)\\s*\\)$"));
    const QRegularExpressionMatch pm = pal.match(value);
    if (pm.hasMatch()) {
        const QString role = pm.captured(1).toLower();
        QPalette::ColorRole cr = QPalette::NoRole;
        if (role == "highlight")       cr = QPalette::Highlight;
        else if (role == "button")     cr = QPalette::Button;
        else if (role == "mid")        cr = QPalette::Mid;
        else if (role == "dark")       cr = QPalette::Dark;
        else if (role == "window")     cr = QPalette::Window;
        else if (role == "base")       cr = QPalette::Base;
        else if (role == "text")       cr = QPalette::Text;
        if (cr != QPalette::NoRole && qApp)
            return qApp->palette().color(cr);
        return QColor();
    }

    const QColor direct(value);
    if (direct.isValid())
        return direct;

    static const QRegularExpression varUse(
        QStringLiteral("var\\(\\s*(--[\\w-]+)\\s*\\)"));
    const QRegularExpressionMatch vm = varUse.match(value);
    if (vm.hasMatch())
        return toColor(sheet, resolveVar(sheet, vm.captured(1)));
    return QColor();
}

}

BoxBorder GetThemeBoxBorder()
{
    static BoxBorder cache;
    static QString lastSheet;
    static bool loaded = false;

    const QString sheet = qApp ? qApp->styleSheet() : QString();
    if (loaded && sheet == lastSheet)
        return cache;

    lastSheet = sheet;
    loaded = true;

    BoxBorder b;

    QString radiusVal =
        resolveVar(sheet, QStringLiteral("--border_radius"));
    if (radiusVal.isEmpty()) {
        static const QRegularExpression rr(
            QStringLiteral("border-radius\\s*:\\s*(\\d+)px"));
        const QRegularExpressionMatch m = rr.match(sheet);
        if (m.hasMatch())
            radiusVal = m.captured(1) + QStringLiteral("px");
    }
    if (!radiusVal.isEmpty()) {
        static const QRegularExpression num(QStringLiteral("(\\d+)"));
        const QRegularExpressionMatch m = num.match(radiusVal);
        if (m.hasMatch())
            b.radius = m.captured(1).toInt();
    }

    QString colorVal =
        resolveVar(sheet, QStringLiteral("--border_color"));
    if (colorVal.isEmpty()) {
        static const QRegularExpression br(
            QStringLiteral("border\\s*:\\s*(\\d+)px\\s+solid\\s+([^;]+);"));
        const QRegularExpressionMatch m = br.match(sheet);
        if (m.hasMatch())
            colorVal = m.captured(2).trimmed();
    }
    if (!colorVal.isEmpty()) {
        const QColor c = toColor(sheet, colorVal);
        if (c.isValid())
            b.color = c;
    }

    static const QRegularExpression bw(
        QStringLiteral("border\\s*:\\s*(\\d+)px\\s+solid"));
    const QRegularExpressionMatch wm = bw.match(sheet);
    if (wm.hasMatch() && wm.captured(1).toInt() > 0)
        b.width = wm.captured(1).toInt();

    cache = b;
    return b;
}

QColor GetPanelLineColor()
{
    static QColor cache;
    static QString lastSheet;
    static bool loaded = false;

    const QString sheet = qApp ? qApp->styleSheet() : QString();
    if (loaded && sheet == lastSheet)
        return cache;

    lastSheet = sheet;
    loaded = true;

    QColor result;

    static const QRegularExpression rule(
        QStringLiteral("QDockWidget::title\\s*\\{([^}]*)\\}"));
    const QRegularExpressionMatch m = rule.match(sheet);
    if (m.hasMatch()) {
        static const QRegularExpression bg(QStringLiteral(
            "(?:background-color|background)\\s*:\\s*([^;]+);"));
        const QRegularExpressionMatch b = bg.match(m.captured(1));
        if (b.hasMatch())
            result = toColor(sheet, b.captured(1).trimmed());
    }

    if (!result.isValid())
        result = GetThemeBoxBorder().color;

    cache = result;
    return result;
}

MixerStateStyle GetThemeMixerStateStyle(const char *stateClass)
{
    static QString lastSheet;
    static QString lastKey;
    static MixerStateStyle lastStyle;
    static bool hasCache = false;

    const QString sheet = qApp ? qApp->styleSheet() : QString();
    const QString key = QString::fromUtf8(stateClass ? stateClass : "");

    if (hasCache && sheet == lastSheet && key == lastKey)
        return lastStyle;

    lastSheet = sheet;
    lastKey = key;
    hasCache = true;

    MixerStateStyle st;

    const QString selector = (stateClass && *stateClass)
        ? QStringLiteral("VolumeControl\\.%1\\s+\\.mixer-category").arg(key)
        : QStringLiteral("VolumeControl\\s+\\.mixer-category");

    const QRegularExpression rule(
        selector + QStringLiteral("[^{]*\\{([^}]*)\\}"),
        QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m = rule.match(sheet);
    if (m.hasMatch()) {
        const QString decls = m.captured(1);

        static const QRegularExpression bgRe(QStringLiteral(
            "(?:background-color|background)\\s*:\\s*([^;]+);"));
        const QRegularExpressionMatch bgm = bgRe.match(decls);
        if (bgm.hasMatch())
            st.bg = toColor(sheet, bgm.captured(1).trimmed());

        static const QRegularExpression fgRe(QStringLiteral(
            "(?<![-\\w])color\\s*:\\s*([^;]+);"));
        const QRegularExpressionMatch fgm = fgRe.match(decls);
        if (fgm.hasMatch())
            st.fg = toColor(sheet, fgm.captured(1).trimmed());

        st.found = st.bg.isValid() && st.fg.isValid();
    }

    lastStyle = st;
    return st;
}

}
