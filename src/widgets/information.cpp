#include "information.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <obs-module.h>

Information::Information(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->setAlignment(Qt::AlignCenter);

    auto createBox = [&](QLabel *&label, const QString &emoji) {
        QWidget *box = new QWidget(this);

        box->setStyleSheet("border: none;");

        auto *boxLayout = new QHBoxLayout(box);
        boxLayout->setContentsMargins(10, 0, 10, 0);
        boxLayout->setSpacing(6);
        boxLayout->setAlignment(Qt::AlignCenter);

        label = new QLabel(emoji + " -", box);
        label->setAlignment(Qt::AlignCenter);
        boxLayout->addWidget(label);

        return box;
    };


    QWidget *boxType     = createBox(typeLabel,   "🧩");
    QWidget *boxOriginal = createBox(sizeLabel,   "📐");
    QWidget *boxScaled   = createBox(scaledLabel, "🔍");
    QWidget *boxUsed     = createBox(usedLabel,   "🗂️");

    mainLayout->addWidget(boxType);
    mainLayout->addWidget(boxOriginal);
    mainLayout->addWidget(boxScaled);
    mainLayout->addWidget(boxUsed);
}

void Information::updateInfo(const QString &name,
                             const QString &type,
                             const QString &size,
                             const QString &scaled,
                             const QString &used)
{
    Q_UNUSED(name);

    typeLabel->setText("🧩 " + type);
    sizeLabel->setText("📐 " + size);
    scaledLabel->setText("🔍 " + scaled);
    usedLabel->setText("🗂️ " + QString(obs_module_text("UsedIn")).arg(used));

}

