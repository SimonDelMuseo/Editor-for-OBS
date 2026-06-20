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
    auto *box = new Box(this);

    /* Crear subwidgets */
    positionBox = new EditPosition(box);
    boundsBox   = new EditBounds(box);
    cropBox     = new EditCrop(box);

    /* Layout interno del Box */
    auto *inner = qobject_cast<QVBoxLayout*>(box->layout());
    if (inner) {
        inner->addWidget(positionBox);
        inner->addWidget(boundsBox);
        inner->addWidget(cropBox);
    }

    /* Layout principal */
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->addWidget(box);
}
