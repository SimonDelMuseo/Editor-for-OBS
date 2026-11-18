#include "moc_plain-text-edit.cpp"
#include <QFontDatabase>

OBSPlainTextEdit::OBSPlainTextEdit(QWidget *parent, bool monospace) : QPlainTextEdit(parent)
{
	document()->setDefaultStyleSheet("font { white-space: pre; }");

	if (monospace) {
		const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

		setStyleSheet(QString("font-family: %1; font-size: %2pt;")
				      .arg(fixedFont.family(), QString::number(fixedFont.pointSize())));
	}
}
