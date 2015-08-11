#pragma once

#include <QWidget>
#include <QString>
#include <stdint.h>
#include <vector>
#include <map>

class MonospaceGridModel : public QObject {
    Q_OBJECT
    
public:
    virtual QString getCell(uint64_t row, int col) = 0;
    virtual uint64_t getMinRow() = 0;
    virtual uint64_t getMaxRow() = 0;
    virtual uint64_t getColumnCount() = 0;
    virtual uint64_t getRowStep();
    void update();
    void navigate(uint64_t row);
    ~MonospaceGridModel();
signals:
    void navigated(uint64_t row);
    void rowHighlighted(uint64_t row);
    void highligtingCleared(uint64_t row);
    void updated();
};

struct ColumnInfo {
    int charsWidth;
};

struct RowInfo {
    bool isSelected;
    bool isHighlighted;
};

class MonospaceGrid : public QWidget {
    Q_OBJECT
    
    MonospaceGridModel* _model = nullptr;
    std::vector<ColumnInfo> _columns;
    std::map<uint64_t, RowInfo> _rows;
    uint64_t _curRow = 0;
    
    virtual void paintEvent(QPaintEvent*) override;
    
public:
    void go(uint64_t row);
    void setModel(MonospaceGridModel* model);
    void setColumnWidth(int col, int chars);
    void selectRow(uint64_t row);
    void highlightRow(uint64_t row);
    void clearHighlights();
};