#pragma once

#include <QWidget>
#include <QColor>

class EditorModel;
class CopyPaste;
class SourceColor;
class Transitions;
class AudioSettings;
class ImageSettings;
class VideoSettings;

class SourceTab : public QWidget
{
    Q_OBJECT

public:
    explicit SourceTab(EditorModel *model, QWidget *parent = nullptr);

private:
    void refreshUI();

    /* ⭐ NUEVO: aplicar color personalizado (ConfirmColor) */
    void applyCustomColor(const QColor &color);

    EditorModel *model = nullptr;

    CopyPaste     *copyPaste     = nullptr;
    SourceColor   *sourceColor   = nullptr;
    Transitions   *transitions   = nullptr;   
    ImageSettings *imageSettings = nullptr;
    VideoSettings *videoSettings = nullptr;
    AudioSettings *audioSettings = nullptr;
};
