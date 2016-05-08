#pragma once

#include "GenericTableModel.h"
#include <QTreeWidgetItem>

struct RsxContext;

class ContextTreeItem : public QTreeWidgetItem {
public:
    ContextTreeItem(std::string name);
    virtual GenericTableModel<RsxContext>* getTable(RsxContext* context) = 0;
};

class SurfaceContextTreeItem : public ContextTreeItem {
public:
    SurfaceContextTreeItem();
    virtual GenericTableModel<RsxContext>* getTable(RsxContext* context) override;
};

class SamplerContextTreeItem : public ContextTreeItem {
    bool _fragment;
    int _index;
public:
    SamplerContextTreeItem(bool fragment, int index);
    virtual GenericTableModel<RsxContext>* getTable(RsxContext* context) override;
};

class SamplerTextureContextTreeItem : public ContextTreeItem {
    bool _fragment;
    int _index;
public:
    SamplerTextureContextTreeItem(bool fragment, int index);
    virtual GenericTableModel<RsxContext>* getTable(RsxContext* context) override;
};