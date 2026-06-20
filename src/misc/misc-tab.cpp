/* ────────────────────────────────────────────────
   INCLUDES
   ──────────────────────────────────────────────── */
#include "misc-tab.hpp"
#include "editor-model.hpp"

#include <QLabel>
#include <QVBoxLayout>

/* ────────────────────────────────────────────────
   CONSTRUCTOR
   ──────────────────────────────────────────────── */
MiscTab::MiscTab(EditorModel *model, QWidget *parent)
    : QWidget(parent)
    , model(model)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Misc Tab"));
    setLayout(layout);
}
