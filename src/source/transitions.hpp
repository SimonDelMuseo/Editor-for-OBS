#pragma once

#include <QWidget>

class Box;
class QLabel;
class QPushButton;
class QComboBox;
class QSpinBox;
class EditorModel;

class Transitions : public QWidget
{
    Q_OBJECT

public:
    explicit Transitions(EditorModel *model, QWidget *parent = nullptr);

    void updateShowLabel(const QString & = QString());
    void updateHideLabel(const QString & = QString());

private:
    void buildShowTransition(Box *box);
    void buildHideTransition(Box *box);

    void connectShowTransition();
    void connectHideTransition();
    void connectConfigButton();
    void connectTimeControl();
    void connectTimeControlHide();

    void refreshControlStates();

    EditorModel *model = nullptr;

    bool m_applyingTransition = false;

    QComboBox   *labelShow = nullptr;
    QSpinBox    *timeSpin = nullptr;
    QPushButton *btnConfig = nullptr;
    QPushButton *btnCopy = nullptr;
    QPushButton *btnPaste = nullptr;

    QComboBox   *labelHide = nullptr;
    QSpinBox    *timeSpinHide = nullptr;
    QPushButton *btnConfigHide = nullptr;
    QPushButton *btnCopyHide = nullptr;
    QPushButton *btnPasteHide = nullptr;
};
