#pragma once

#include <QLineEdit>

class CommandLineEdit : public QLineEdit {
    Q_OBJECT
    
    virtual bool eventFilter(QObject*, QEvent*);
public:
    CommandLineEdit(QWidget* parent);
signals:
    void selected();
};