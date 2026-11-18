#pragma once

#include <QAction>
#include <QAbstractButton>
#include <QColor>
#include <QIcon>
#include <QString>
#include <string>

namespace editorui {

std::string GetThemeTypeName();

void SetButtonIcon(QAbstractButton *button, const char *path);

std::string GetIconPath(const std::string &iconName);

void RefreshButtonIcons();

struct BoxBorder {
    int width  = 1;
    QColor color{0x80, 0x80, 0x80};
    int radius = 4;
};

BoxBorder GetThemeBoxBorder();

QColor GetPanelLineColor();

struct MixerStateStyle {
    QColor bg;
    QColor fg;
    bool found = false;
};
MixerStateStyle GetThemeMixerStateStyle(const char *stateClass);


QAction *FindNativeAction(const char *name);

bool TriggerNativeAction(const char *name);

bool InvokeNativeMethod(const char *method);

}
