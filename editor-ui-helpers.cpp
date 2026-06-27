#include "editor-ui-helpers.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QIcon>
#include <QString>
#include <QDir>

namespace editorui {

std::string GetThemeTypeName()
{
    return obs_frontend_is_theme_dark() ? "Dark" : "Light";
}

void SetButtonIcon(QAbstractButton *button, const char *path)
{
    QIcon icon;
    icon.addFile(QString::fromUtf8(path), QSize(), QIcon::Normal, QIcon::Off);
    button->setIcon(icon);
}

std::string GetIconPath(const std::string &iconName)
{
    // Ruta absoluta al archivo del módulo (DLL del plugin)
    char *modulePath = obs_module_file(nullptr);

    // Convertimos a QString para manipular rutas
    QString base = QString::fromUtf8(modulePath);
    bfree(modulePath);

    // Subimos un nivel: /obs-plugins/64bit/E33-Editor.dll → /obs-plugins/64bit/
    QDir dir(base);
    dir.cdUp();

    // Ahora bajamos a la carpeta data del plugin:
    // <OBS>/data/obs-plugins/E33-Editor/icons/
    dir.cd("../../data/obs-plugins/E33-Editor/icons/");

    // Tema actual ("Dark" o "Light")
    std::string theme = GetThemeTypeName();

    // Construcción final: DarkTransform.svg, LightTransform.svg, etc.
    QString fullPath = dir.filePath(QString::fromStdString(theme + iconName));

    return fullPath.toStdString();
}

} // namespace editorui
