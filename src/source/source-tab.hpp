#pragma once

#include <QWidget>
#include <QColor>
#include <QMap>

class EditorModel;
class CopyPaste;
class SourceColor;
class Transitions;
class AudioSettings;
class ImageSettings;
class VideoSettings;
class QPropertyAnimation;

class SourceTab : public QWidget
{
    Q_OBJECT

public:
    explicit SourceTab(EditorModel *model, QWidget *parent = nullptr);

private:
    void refreshUI();

    void setContainerVisible(QWidget *box, bool wantVisible);
    void startBoxAnim(QWidget *box, int from, int to, bool hideAtEnd);

    void applyCustomColor(const QColor &color);

    EditorModel *model = nullptr;

    CopyPaste     *copyPaste     = nullptr;
    SourceColor   *sourceColor   = nullptr;
    Transitions   *transitions   = nullptr;   
    ImageSettings *imageSettings = nullptr;
    VideoSettings *videoSettings = nullptr;
    AudioSettings *audioSettings = nullptr;

    QMap<QWidget *, QPropertyAnimation *> boxAnims;
};
