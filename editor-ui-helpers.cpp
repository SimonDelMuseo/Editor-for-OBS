#include "editor-ui-helpers.hpp"

#include <obs-frontend-api.h>
#include <QIcon>
#include <QString>

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
    // Ruta base de tus iconos (ajustada a tu estructura real)
    static const std::string basePath =
        "E:/OBS-PLUGIN/E33-Editor/data/obs-plugins/E33-Editor/icons/";

    // Tema actual
    std::string theme = GetThemeTypeName(); // "Dark" o "Light"

    // Ejemplo final:  E:/.../icons/DarkCopy.svg
    return basePath + theme + iconName;
}

} // namespace editorui
