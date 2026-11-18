#include "sign.hpp"
#include "editor-model.hpp"
#include "editor-ui-helpers.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>

static constexpr int kSignHeight = 23;
static constexpr int kSignAnimMs = 300;

Sign::Sign(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    setMaximumHeight(0);

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    label = new QLabel(this);
    label->setTextFormat(Qt::RichText);
    label->setAlignment(Qt::AlignCenter);
    label->setOpenExternalLinks(true);
    label->setFixedHeight(kSignHeight);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    label->setText(QStringLiteral(
        "<a href=\"https://github.com/SimonDelMuseo/Editor-for-OBS/releases\">"
        "Editor</a> (" E33_VERSION ") by "
        "<a href=\"https://simondelmuseo.github.io/estudio33/\">SimónDelMuseo</a>"));
    lay->addWidget(label);

    anim = new QPropertyAnimation(this, "maximumHeight", this);
    anim->setDuration(kSignAnimMs);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QPropertyAnimation::finished, this, [this]() {
        if (!shown)
            hide();
    });

    if (model)
        connect(model, &EditorModel::modelUpdated,
                this, &Sign::refreshState);

    refreshState();
}

void Sign::refreshState()
{
    const bool wantShow = !model || model->selectedSource() == nullptr;

    if (firstRefresh) {
        firstRefresh = false;
        shown = wantShow;
        if (shown) {
            setMaximumHeight(kSignHeight);
            show();
        } else {
            setMaximumHeight(0);
            hide();
        }
        return;
    }

    animateTo(wantShow);
}

void Sign::animateTo(bool wantShow)
{
    if (wantShow == shown)
        return;

    shown = wantShow;
    anim->stop();

    anim->setStartValue(maximumHeight());
    anim->setEndValue(wantShow ? kSignHeight : 0);
    if (wantShow && !isVisible())
        show();
    anim->start();
}

void Sign::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);
    p.setPen(QPen(editorui::GetPanelLineColor(), 1));
    p.drawLine(0, 0, width(), 0);
}