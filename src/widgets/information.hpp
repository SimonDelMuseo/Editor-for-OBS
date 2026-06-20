#pragma once
#include <QWidget>
#include <QLabel>

class QVBoxLayout;
class QGridLayout;
class Box;

class Information : public QWidget
{
    Q_OBJECT

public:
    explicit Information(QWidget *parent = nullptr);

    void updateInfo(const QString &name,
                    const QString &type,
                    const QString &size,
                    const QString &scaled,
                    const QString &used);

private:
    QLabel *nameLabel   = nullptr;
    QLabel *typeLabel   = nullptr;
    QLabel *sizeLabel   = nullptr;
    QLabel *scaledLabel = nullptr;
    QLabel *usedLabel   = nullptr;
};
