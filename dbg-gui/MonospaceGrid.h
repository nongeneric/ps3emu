#pragma once

#include <QWidget>
#include <QString>
#include <QPainter>
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
    virtual bool pointsTo(uint64_t row, uint64_t& to, bool& highlighted);
    void navigate(uint64_t row);
    ~MonospaceGridModel();
signals:
    void navigated(uint64_t row);
    void updated();
};

enum class NavigationMode {
    // just navigate ignoring the current position
    Strict,
    // don't navigate if already on screen, works well for disassembly
    // where every address is always aligned and there is no need to
    // to change the starting address of a current line
    Fuzzy
};

struct ColumnInfo {
    int charsWidth;
};

struct RowInfo {
    bool isSelected;
    bool isHighlighted;
};

struct ArrowInfo {
    uint64_t from;
    uint64_t to;
    bool highlighted;
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
    int _arrowsColumn = -1;
    NavigationMode _navigationMode = NavigationMode::Strict;
    
    virtual void paintEvent(QPaintEvent*) override;
    void navigate(uint64_t row);
    void drawArrow(ArrowInfo arrow, int pos, QPainter& painter);
    void drawArrows(std::vector<ArrowInfo> arrows, QPainter& painter);
    int getColumnWidth(int index);
public:
    void setModel(MonospaceGridModel* model);
    void setColumnWidth(int col, int chars);
    void setScrollable(bool value);
    void setArrowsColumn(int col);
    void setNavigationMode(NavigationMode mode);
protected:
    virtual void wheelEvent(QWheelEvent *) override;
};