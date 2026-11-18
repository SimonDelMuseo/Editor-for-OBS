#include "edit.hpp"
#include "edit-position.hpp"
#include "edit-bounds.hpp"
#include "edit-crop.hpp"

#include "src/widgets/box.hpp"

#include <QVBoxLayout>
#include <QLabel>

EditBox::EditBox(QWidget *parent)
    : QWidget(parent)
{
    auto *boxTransform = new Box(this);
    auto *boxBounds    = new Box(this);
    auto *boxCrop      = new Box(this);

    positionBox = new EditPosition(this);
    boundsBox   = new EditBounds(this);
    cropBox     = new EditCrop(this);

    auto put = [](Box *box, QWidget *w) {
        if (auto *inner = qobject_cast<QVBoxLayout *>(box->layout()))
            inner->addWidget(w);
    };
    put(boxTransform, positionBox);
    put(boxBounds,    boundsBox);
    put(boxCrop,      cropBox);

    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->addWidget(boxTransform);
    main->addWidget(boxBounds);
    main->addWidget(boxCrop);
}
