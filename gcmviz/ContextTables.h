#pragma once

#include "GenericTableModel.h"
#include <QTreeWidgetItem>

struct RsxContext;

class ContextTreeItem : public QTreeWidgetItem {
public:
    ContextTreeItem(QString name)
        : QTreeWidgetItem((QTreeWidget*)nullptr, QStringList(name)) {}
    virtual GenericTableModel<RsxContext>* getTable(RsxContext* context) = 0;
};

class SurfaceContextTreeItem : public ContextTreeItem {
public:
    SurfaceContextTreeItem();
    virtual GenericTableModel<RsxContext>* getTable(RsxContext* context) override;
};