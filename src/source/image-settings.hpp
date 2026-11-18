#pragma once
#include <QWidget>
#include "src/widgets/box.hpp"

class QComboBox;
class QPushButton;
class QMenu;
class EditorModel;

class ImageSettings : public Box
{
    Q_OBJECT

public:
    explicit ImageSettings(QWidget *parent = nullptr);

    void setModel(EditorModel *m) { model = m; }

    void setScaleFiltering(const QString &value);

    QComboBox *scaleFilteringCombo = nullptr;
    QComboBox *blendingModeCombo = nullptr;
    QComboBox *blendingMethodCombo = nullptr;

signals:
    void openProjectorOnMonitorRequested(int monitorIndex);
    void openProjectorWindowRequested();

    void scaleFilteringChanged(const QString &value);

private:
    QPushButton *saveScreenshotButton = nullptr;

    QPushButton *projectorMenuButton = nullptr;
    QMenu *projectorMenu = nullptr;

    EditorModel *model = nullptr;
};
