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
    virtual int getColumnCount() = 0;
    virtual uint64_t getRowStep();
    virtual void update();
    virtual bool isHighlighted(uint64_t row, int col);
    void navigate(uint64_t row);
    ~MonospaceGridModel();
signals:
    void navigated(uint64_t row);
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
    
    bool _scrollable = false;
    MonospaceGridModel* _model = nullptr;
    std::vector<ColumnInfo> _columns;
    uint64_t _curRow = 0;
    uint64_t _lastRow = 0;
    int _charWidth;
    int _charHeight;
    
    virtual void paintEvent(QPaintEvent*) override;
    void navigate(uint64_t row);
public:
    void setModel(MonospaceGridModel* model);
    void setColumnWidth(int col, int chars);
    void setScrollable(bool value);
protected:
    virtual void wheelEvent(QWheelEvent *) override;
};