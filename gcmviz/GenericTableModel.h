#pragma once

#include <QAbstractItemModel>
#include <string>
#include <functional>

template<class T>
struct GenericTableRow {
    std::string name;
    std::function<std::string(T*)> eval;
    std::function<std::string(T*)> altEval;
};

template<class T>
class GenericTableModel : public QAbstractItemModel {
    T* _info;
    std::vector<GenericTableRow<T>> _rows;
public:
    GenericTableModel(T* info, std::vector<GenericTableRow<T>> rows)
        : _info(info), _rows(rows) {}
    
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return QVariant();
        switch (section) {
            case 0: return "Name";
            case 1: return "Value";
            case 2: return "Alternative Value";
        }
        return QVariant();
    }
    
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 3;
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole)
            return QVariant();
        switch (index.column()) {
            case 0: return QString::fromStdString(_rows[index.row()].name);
            case 1: return QString::fromStdString(_rows[index.row()].eval(_info));
            case 2: return QString::fromStdString(_rows[index.row()].altEval(_info));
        }
        return QVariant();
    }
    
    QModelIndex parent(const QModelIndex& child) const override {
        return QModelIndex();
    }
    
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override {
        return createIndex(row, column);
    }
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return _rows.size();
    }
};
