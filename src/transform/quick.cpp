#include "quick.hpp"
#include "src/widgets/box.hpp"
#include "editor-ui-helpers.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <obs-module.h>

static QLabel *makeSubtitleLabel(const char *key, QWidget *parent)
{
    auto *lbl = new QLabel(obs_module_text(key), parent);
    lbl->setProperty("class", "subtitle");
    return lbl;
}

QuickBox::QuickBox(QWidget *parent)
    : QWidget(parent)
{
    auto *box = new Box(this);

    auto *line = new QHBoxLayout();
    line->setContentsMargins(0, 0, 0, 0);
    line->setSpacing(8);

    line->addWidget(makeSubtitleLabel("RotateTitle", box));

    auto *btnRotCW = new QPushButton(box);
    btnRotCW->setObjectName("rotate90RightButton");
    btnRotCW->setFixedHeight(28);
    btnRotCW->setIconSize(QSize(18, 18));
    btnRotCW->setText(obs_module_text("Rotate90CW"));
    btnRotCW->setToolTip(obs_module_text("Rotate90CWTip"));
    btnRotCW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editorui::SetButtonIcon(
        btnRotCW,
        editorui::GetIconPath("Rotate90-right.svg").c_str()
    );
    line->addWidget(btnRotCW);

    auto *btnRotCCW = new QPushButton(box);
    btnRotCCW->setObjectName("rotate90LeftButton");
    btnRotCCW->setFixedHeight(28);
    btnRotCCW->setIconSize(QSize(18, 18));
    btnRotCCW->setText(obs_module_text("Rotate90CCW"));
    btnRotCCW->setToolTip(obs_module_text("Rotate90CCWTip"));
    btnRotCCW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editorui::SetButtonIcon(
        btnRotCCW,
        editorui::GetIconPath("Rotate90-left.svg").c_str()
    );
    line->addWidget(btnRotCCW);

    auto *btnRot180 = new QPushButton(box);
    btnRot180->setObjectName("rotate180Button");
    btnRot180->setFixedHeight(28);
    btnRot180->setIconSize(QSize(18, 18));
    btnRot180->setText(obs_module_text("Rotate180"));
    btnRot180->setToolTip(obs_module_text("Rotate180Tip"));
    btnRot180->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editorui::SetButtonIcon(
        btnRot180,
        editorui::GetIconPath("Rotate180.svg").c_str()
    );
    line->addWidget(btnRot180);

    line->addSpacing(8);

    line->addWidget(makeSubtitleLabel("FlipTitle", box));

    auto *btnFlipH = new QPushButton(box);
    btnFlipH->setObjectName("flipHorizontalButton");
    btnFlipH->setIconSize(QSize(18, 18));
    btnFlipH->setToolTip(obs_module_text("FlipHorizontal"));
    btnFlipH->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnFlipH->setFixedSize(28, 28);
    btnFlipH->setStyleSheet(QStringLiteral(
        "#flipHorizontalButton { height: 28px; max-height: 28px;"
        " min-height: 28px; margin: 0px; padding: 0px; }"));
    editorui::SetButtonIcon(
        btnFlipH,
        editorui::GetIconPath("Flip-horizontal.svg").c_str()
    );
    line->addWidget(btnFlipH);

    auto *btnFlipV = new QPushButton(box);
    btnFlipV->setObjectName("flipVerticalButton");
    btnFlipV->setIconSize(QSize(18, 18));
    btnFlipV->setToolTip(obs_module_text("FlipVertical"));
    btnFlipV->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnFlipV->setFixedSize(28, 28);
    btnFlipV->setStyleSheet(QStringLiteral(
        "#flipVerticalButton { height: 28px; max-height: 28px;"
        " min-height: 28px; margin: 0px; padding: 0px; }"));
    editorui::SetButtonIcon(
        btnFlipV,
        editorui::GetIconPath("Flip-vertical.svg").c_str()
    );
    line->addWidget(btnFlipV);

    auto *inner = qobject_cast<QVBoxLayout*>(box->layout());
    if (inner)
        inner->addLayout(line);

    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->addWidget(box);

    connect(btnFlipH, &QPushButton::clicked, this, &QuickBox::flipHorizontal);
    connect(btnFlipV, &QPushButton::clicked, this, &QuickBox::flipVertical);

    connect(btnRotCW,  &QPushButton::clicked, this, &QuickBox::rotate90CW);
    connect(btnRotCCW, &QPushButton::clicked, this, &QuickBox::rotate90CCW);
    connect(btnRot180, &QPushButton::clicked, this, &QuickBox::rotate180);
}
