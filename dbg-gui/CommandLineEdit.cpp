#include "CommandLineEdit.h"

#include <QKeyEvent>

bool CommandLineEdit::eventFilter(QObject* o, QEvent* e) {
    if (e->type() == QEvent::KeyPress) {
        auto ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
            emit selected();
            return true;
        }
    }
    return QLineEdit::eventFilter(o, e);
}

CommandLineEdit::CommandLineEdit(QWidget* parent) : QLineEdit(parent) {
    installEventFilter(this);
}
