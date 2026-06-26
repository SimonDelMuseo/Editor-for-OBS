#pragma once

#include <QAbstractButton>
#include <QColor>
#include <QIcon>
#include <QString>
#include <string>

namespace editorui {

// Devuelve "Dark" o "Light" según el tema actual de OBS
std::string GetThemeTypeName();

// Asigna un icono SVG al botón desde una ruta absoluta
void SetButtonIcon(QAbstractButton *button, const char *path);

// Construye automáticamente la ruta completa del icono dinámico
std::string GetIconPath(const std::string &iconName);

} // namespace editorui
